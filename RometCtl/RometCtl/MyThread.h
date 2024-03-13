#pragma once
#include <atomic>
#include <vector>
#include "pch.h"
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();//ThreadFuncBase的成员函数指针
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
	bool IsValid() const{//判断是否有效
		return (thiz != NULL) && (func != NULL);
	}
private:
	ThreadFuncBase* thiz;//ThreadFuncBase对象指针
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
	//线程是否还活着,true 表示有效 false 表示线程异常或者已经终止
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
	//判断是不是空闲的,true表示空闲
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
			//load 函数用于获取原子变量的当前值
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
			else {//没有命令处理时，该线程空转，不会关闭，避免_beginthread重复执行造成的性能浪费
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
	bool m_bStatus;//false 表示线程将要关闭  true 表示线程正在运行
	std::atomic<::ThreadWork*> m_worker;
};

//线程池
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
	//分发worker，返回-1表示分配失败，所有线程都在忙
	int DispatchWorker(const ThreadWork& worker) {
		int index = -1;
		m_lock.lock();
		//轮询的方式
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
	//检测线程的有效性
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