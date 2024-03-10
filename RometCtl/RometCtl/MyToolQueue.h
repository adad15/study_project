#pragma once
#include "pch.h"
#include <mutex>
#include <atomic>

template<class T>
class CMyToolQueue
{//�̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	enum {
		MTNone,
		MTPush,
		MTPop,
		MTSize,
		MTClear
	};
	typedef struct IocpParam {
		size_t nOperator;//����
		T Data;//����
		HANDLE hEvent;//pop ������Ҫ��
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = MTNone;
		}
	}PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��

public:
	CMyToolQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE/*���*/, NULL, NULL, 1/*�ܹ�ͬʱ���ʵ��߳���*/);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(CMyToolQueue<T>::threadEntry, 0, m_hCompeletionPort);
		}
	}
	~CMyToolQueue() {
		//��ֹ������������
		if (m_lock) return;
		m_lock = true;
		//�����Ա��
		HANDLE hTemp = m_hCompeletionPort;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);//���޵ȴ��߳̽���
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(MTPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		} 
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//ʧ���˾Ͳ�����ɶ˿ڽ���delte����Ϊ��ɶ˿�û�б�ִ��
		return ret;
	}
	bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(MTPop, data, hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		} 
		//���ﲻ�ʺ�ʹ�ûص���������Ϊ������ȡֵ����Ȼ��Ҫ�ȵ����ݣ�����û��ã���Ȼ��Ҫ�ȴ���
		//������������Ҫ��ô��ơ�д���ݾͲ���Ҫ�ȴ��ˡ�
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		//m_lstData.size() ���̰߳�ȫ������
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(MTSize, T(), hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return -1;
		} 
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		IocpParam* pParam = new IocpParam(MTClear, T());
		if (m_lock) {
			delete pParam;
			return false;
		} 
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//ʧ���˾Ͳ�����ɶ˿ڽ���delte����Ϊ��ɶ˿�û�б�ִ��
		return ret;
	}
private:
	static void threadEntry(void* arg) {
		CMyToolQueue<T>* thiz = (CMyToolQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void DealParam(PPARAM* pParam) {
		//��ʽ����������ʹ���첽���ݣ�������ʹ��CompletionKey��������ʾ��
		switch (pParam->nOperator)
		{
		case MTPush:
		{
			m_lstData.push_back(pParam->Data);
			delete pParam;
		}
		break;
		case MTPop:
		{
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
		}
		break;
		case MTSize:
		{
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
		}
		break;
		case MTClear:
		{
			m_lstData.clear();
			delete pParam;
		}
		break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	void threadMain() {
		DWORD dwTransferred{};
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey{};
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE/*���޵ȴ�*/))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare is exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//�����ݿ϶��Ͳ��ᳬʱ
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare is exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		CloseHandle(m_hCompeletionPort);
	}
private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//ԭ�ӱ���
};