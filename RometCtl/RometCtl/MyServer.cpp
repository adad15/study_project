#include "pch.h"
#include "MyServer.h"
#pragma warning(disable:4407)

template<MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_worker = ThreadWork(this, (FUNCTYPE) & AcceptOverlapped<op>::AcceptWorker);
	m_operator = MyAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_pserver = NULL;
}

template<MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength{}, rLength{};
	//使用CMyClient::operator LPDWORD()
	if (*(LPDWORD)*m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength/*本地地址*/,
			(sockaddr**)m_client->GetRemoteAddr(), &rLength/*远程地址*/);
		
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, & m_client->flags(), *m_client, NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {

		}
		if (!m_pserver->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

template<MyOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = MyRecv;
	m_worker = ThreadWork(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<MyOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = MySend;
	m_worker = ThreadWork(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

CMyClient::CMyClient() :m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}
void CMyClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}
CMyClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF CMyClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}
LPWSABUF CMyClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

bool CMyServer::StartServic()
{
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

int CMyServer::threadIocp()
{
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