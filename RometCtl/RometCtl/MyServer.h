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
template<MyOperator>class RecvOverlapped;
typedef RecvOverlapped<MyRecv>RECVOVERLAPPED;
template<MyOperator>class SendOverlapped;
typedef SendOverlapped<MySend>SENDOVERLAPPED;

class CMyOverlapped {
public:
	OVERLAPPED m_overlapped;
    DWORD m_operator;          //����,�μ�MyOperator
    std::vector<char> m_buffer;//������
    ThreadWork m_worker;//������
    CMyServer* m_pserver;//����������
	CMyClient* m_client;//��Ӧ�Ŀͻ���
	WSABUF m_wsabuffer;
	virtual ~CMyOverlapped() {
		m_buffer.clear();
	}
};

class CMyClient :public ThreadFuncBase {
public:
	CMyClient();
	~CMyClient() {
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_overlapped.reset();
		m_buffer.clear();
		m_sendBuffer.Clear();
	}
	//���ݲ�����CMyClient��ָ��,����
	void SetOverlapped(PCLIENT& ptr);
	//ǿ������ת������CMyClient��ת��ΪSOCKET���͵�m_sock
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() 
	{
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD() 
	{
		return &m_received;
	}

	LPWSABUF RecvWSABuffer();
	LPWSABUF SendWSABuffer();

	DWORD& flags() { return m_flags; }

	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize() const{
		return m_buffer.size();
	}
	//�ӿ�
	int Recv() {
		int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
		if (ret <= 0) return -1;
		m_used += (size_t)ret;
		//TODO �������ݻ�û�����
		return 0;
	}
	int Send(void* buffer, size_t nSize) {
		std::vector<char>data(nSize);
		memcpy(data.data(), buffer, nSize);
		if (m_sendBuffer.PushBack(data))
			return 0;
		return -1;
	}
	int SendData(std::vector<char>& data);
private:

	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char>m_buffer;
	size_t m_used;//�Ѿ�ʹ�õĻ������Ĵ�С
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	MySendQueue<std::vector<char>>m_sendBuffer;//�������ݶ���
};

template<MyOperator>
class AcceptOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
};

template<MyOperator>
class RecvOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
	RecvOverlapped();
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
};

template<MyOperator>
class SendOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
	SendOverlapped();
	int SendWorker() {
		/*
		* 1.send ���ܲ����������
		* 
		*/
		return -1;
	}
};

template<MyOperator>
class ErrorOverlapped :public CMyOverlapped, ThreadFuncBase {
public:
    ErrorOverlapped() :m_operator(MyError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		return -1;
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
    ~CMyServer() {
		closesocket(m_sock);
		std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
		for (; it != m_client.end(); it++) {
			it->second.reset();
		}
		m_client.clear();
		CloseHandle(m_hIOCP);
		m_pool.Stop();
	}
	bool StartServic();

	bool NewAccept() {
		PCLIENT pClient{ new CMyClient() };
		pClient->SetOverlapped(pClient);//�ͻ���ָ����뵽Overlapped����
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		//�ڷ�����������ʱ��Ͷ��һ��AcceptEx
		if (AcceptEx(m_sock, *pClient, *pClient/*ʹ����operator PVOID()*/, 0, sizeof(sockaddr_in) + 16,
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
		//�����׽�������
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	int threadIocp();
private:
    CMyThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<CMyClient>> m_client;//����ӳ���
};