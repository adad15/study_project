#pragma once
#include "MyThread.h"
#include "MyToolQueue.h"
#include <map>
#include <MSWSock.h>

enum MyOperator{
    MyNone,
    MyAccept,
    MyRecv,
    MySend,
    MyError
};
 
class CMyServer;
class CMyClient;
typedef std::shared_ptr<CMyClient> PCLIENT;

template<MyOperator>class AcceptOverlapped;
typedef AcceptOverlapped<MyAccept>ACCEPTOVERLAPPED;

class CMyOverlapped {
public:
	OVERLAPPED m_overlapped;
    DWORD m_operator;          //操作,参见MyOperator
    std::vector<char> m_buffer;//缓冲区
    ThreadWork m_worker;//处理函数
    CMyServer* m_server;//服务器对象
};

class CMyClient {
public:
	CMyClient();
	~CMyClient() {
		closesocket(m_sock);
	}
	void SetOverlapped(PCLIENT& ptr);
	//??????????????强制数据类型转换？？？？？？？？？？？？？
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD();
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr< ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char>m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};

template<MyOperator>
class AcceptOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
    PCLIENT m_client;
};

template<MyOperator>
class RecvOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
    RecvOverlapped() :m_operator(MyRecv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
	}
	int RecvWorker() {

	}
};
typedef RecvOverlapped<MyRecv>RECVOVERLAPPED;

template<MyOperator>
class SendOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
    SendOverlapped() :m_operator(MySend), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
	}
	int SendWorker() {

	}
};
typedef SendOverlapped<MySend>SENDOVERLAPPED;

template<MyOperator>
class ErrorOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
    ErrorOverlapped() :m_operator(MyError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {

	}
};
typedef ErrorOverlapped<MyError>ERROROVERLAPPED;

class CMyServer :public ThreadFuncBase
{
public:
    CMyServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    ~CMyServer() {}
    bool StartServic() {
		CreatSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		//完成端口映射初始化
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);//绑定端口
		m_pool.Invoke();
		m_pool.DispatchWorker(ThreadWork(this, (FUNCTYPE)&CMyServer::threadIocp));
		if (!NewAccept()) return false;
		return true;
    }
	bool NewAccept() {
		PCLIENT pClient{ new CMyClient() };
		pClient->SetOverlapped(pClient);//客户端指针加入到Overlapped里面
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		//在服务器启动的时候投递一个AcceptEx
		if (AcceptEx(m_sock, *pClient, *pClient/*使用了operator PVOID()*/, 0, sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16, *pClient/*operator LPDWORD()*/, *pClient/*operator LPOVERLAPPED()*/) == FALSE)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}
private:
	void CreatSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		//设置套接字属性
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
    int threadIocp() {
        DWORD tranferred{};
        ULONG_PTR CompletionKey{};
        OVERLAPPED* lpOverlapped{ NULL };
        if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
            if (tranferred > 0 && (CompletionKey != 0)) {
				CMyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CMyOverlapped, m_overlapped);
				switch (pOverlapped->m_operator) {
				case MyAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case MyRecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case MySend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case MyError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				}
            }
            else {
                return -1;
            }
        }
        return 0;
    }
private:
    CMyThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<CMyClient>> m_client;//建立映射表
};