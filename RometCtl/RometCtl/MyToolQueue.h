#pragma once
#include "pch.h"
#include <mutex>
#include <atomic>

template<class T>
class CMyToolQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		MTNone,
		MTPush,
		MTPop,
		MTSize,
		MTClear
	};
	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent;//pop 操作需要的
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = MTNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体

public:
	CMyToolQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE/*句柄*/, NULL, NULL, 1/*能够同时访问的线程数*/);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(CMyToolQueue<T>::threadEntry, 0, m_hCompeletionPort);
		}
	}
	~CMyToolQueue() {
		//防止反复调用析构
		if (m_lock) return;
		m_lock = true;
		//防御性编程
		HANDLE hTemp = m_hCompeletionPort;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);//无限等待线程结束
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
		if (ret == false) delete pParam;//失败了就不在完成端口进行delte，因为完成端口没有被执行
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
		//这里不适合使用回调函数，因为这里是取值，自然需要等到数据，数据没填好，自然需要等待。
		//所以拿数据需要这么设计。写数据就不需要等待了。
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		//m_lstData.size() 非线程安全！！！
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
		if (ret == false) delete pParam;//失败了就不在完成端口进行delte，因为完成端口没有被执行
		return ret;
	}
private:
	static void threadEntry(void* arg) {
		CMyToolQueue<T>* thiz = (CMyToolQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void DealParam(PPARAM* pParam) {
		//正式传递数据是使用异步数据，而不是使用CompletionKey。这里是示范
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
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE/*无限等待*/))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare is exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//有数据肯定就不会超时
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
	std::atomic<bool> m_lock;//原子变量
};