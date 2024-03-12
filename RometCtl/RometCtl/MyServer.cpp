#include "pch.h"
#include "MyServer.h"
#pragma warning(disable:4407)
template<MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_worker = ThreadWork(this, (FUNCTYPE) & AcceptOverlapped<op>::AcceptWorker);
	m_operator = MyAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength{}, rLength{};
	//使用CMyClient::operator LPDWORD()
	if (*(LPDWORD)*m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength/*本地地址*/,
			(sockaddr**)m_client->GetRemoteAddr(), &rLength/*远程地址*/);
		PCLIENT pClient{ new CMyClient() };
		//pClient->SetOverlapped(pClient);//客户端指针加入到Overlapped里面
		//m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		//if (AcceptEx(m_sock, *pClient, *pClient/*使用了operator PVOID()*/, 0, sizeof(sockaddr_in) + 16,
		//	sizeof(sockaddr_in) + 16, *pClient/*operator LPDWORD()*/, *pClient/*operator LPOVERLAPPED()*/) == FALSE)
		//{
		//	return -2;
		//}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

CMyClient::CMyClient() :m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED()) {
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}
void CMyClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr;
}
CMyClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}
CMyClient::operator LPDWORD() {
	return &m_received;
}