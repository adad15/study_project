#include "pch.h"
#include "MyServer.h"
#include "MyTool.h"

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
	if (m_client->GetBufferSize() > 0) {
		sockaddr* plocal = NULL, * premote = NULL;
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength/*本地地址*/,
			(sockaddr**)&premote, &rLength/*远程地址*/);
		memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		m_pserver->BindNewSocket(*m_client);

		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, & m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			TRACE("ret=%d error =%d\r\n", ret, WSAGetLastError());
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
	m_send(new SENDOVERLAPPED()),
	m_sendBuffer(this,(SENDCALLBACK)& CMyClient::SendData)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}
void CMyClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}
CMyClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF CMyClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}
LPWSAOVERLAPPED CMyClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}
LPWSAOVERLAPPED CMyClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}
int CMyClient::SendData(std::vector<char>& data)
{
	//看有没有后续的需要send
	if (m_sendBuffer.Size() > 0) {
		//返回0表示成功
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CMyTool::ShowError();
			return -1;
		}
	}
	return 0;
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
		if (CompletionKey != 0) {
			CMyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CMyOverlapped, m_overlapped);
			TRACE("pOverlapped->m_operator %d\r\n", pOverlapped->m_operator);
			pOverlapped->m_pserver = this;
			//交给其他线程来做
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

void CMyServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}
