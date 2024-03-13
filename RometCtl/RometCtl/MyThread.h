#pragma once
#include <atomic>
#include <vector>
#include "pch.h"
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();//ThreadFuncBase�ĳ�Ա����ָ��
class ThreadWork {
public:
	ThreadWork() :thiz(NULL), func(NULL) {}
	ThreadWork(ThreadFuncBase* obj, FUNCTYPE f) :thiz(obj), func(f) {}
	ThreadWork(const ThreadWork& worker)
	{
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWork& operator=(const ThreadWork& worker)
	{
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid() const{//�ж��Ƿ���Ч
		return (thiz != NULL) && (func != NULL);
	}
private:
	ThreadFuncBase* thiz;//ThreadFuncBase����ָ��
	FUNCTYPE func;
};

class CMyThread
{
public:
	CMyThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~CMyThread() {
		Stop();
	}
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CMyThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}
	//�߳��Ƿ񻹻���,true ��ʾ��Ч false ��ʾ�߳��쳣�����Ѿ���ֹ
	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		UpdataWorker();
		return ret;
	}

	void UpdataWorker(const ::ThreadWork& worker = ::ThreadWork()) {
		if (m_worker.load() != NULL && (m_worker.load() != &worker)) {
			::ThreadWork* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker)return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWork(worker));
	}
	//�ж��ǲ��ǿ��е�,true��ʾ����
	bool IsIdle() {
		if (m_worker.load() == NULL) return true;
		return!m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			//load �������ڻ�ȡԭ�ӱ����ĵ�ǰֵ
			::ThreadWork worker = *m_worker.load();
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();
					if (ret != 0) {
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0) {
						m_worker.store(NULL);
					}
				}
			}
			else {//û�������ʱ�����߳̿�ת������رգ�����_beginthread�ظ�ִ����ɵ������˷�
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		CMyThread* thiz = (CMyThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false ��ʾ�߳̽�Ҫ�ر�  true ��ʾ�߳���������
	std::atomic<::ThreadWork*> m_worker;
};

//�̳߳�
class CMyThreadPool {
public:
	CMyThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i{}; i < size; i++) { 
			m_threads[i] = new CMyThread();
		}
	}
	CMyThreadPool() {}
	~CMyThreadPool() {
		Stop();
		for (size_t i{}; i < m_threads.size(); i++) {
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	bool Invoke() { 
		bool ret = true;
 		for (size_t i{}; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret = false) {
			for (size_t i{}; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i{}; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	//�ַ�worker������-1��ʾ����ʧ�ܣ������̶߳���æ
	int DispatchWorker(const ThreadWork& worker) {
		int index = -1;
		m_lock.lock();
		//��ѯ�ķ�ʽ
		for (size_t i{}; i < m_threads.size(); i++) {
			if (m_threads[i]->IsIdle()) {
				m_threads[i]->UpdataWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}
	//����̵߳���Ч��
	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}

private:
	std::mutex m_lock;
	std::vector<CMyThread*> m_threads;
};