#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		//��̬��Ա����û��thisָ�룬�޷�ֱ�ӷ���˽�ó�Ա����
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}

		return m_instance;
	}
	bool InitSocket() {
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		
		if (listen(m_sock, 1) == -1) return false;//ֻ����һ�����ƶ˽��п��ƣ�������Ϊ1

		return true;
	}
	bool AcceptClient() {
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		if (m_client == -1) return false;
		//closesocket(m_client);
		return true;
	}
	int DealCommond(){
		if (m_client == -1) return false;
		char buffer[1024]{ "" };
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			//TODO:��������

		}
	}
	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

private:
	SOCKET m_sock;
	SOCKET m_client;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		//�׽��ֱ�����ʼ��
		m_client = INVALID_SOCKET;  //��������-1.

		//��ʼ���׽��ֻ���
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		//�׽��ֱ�����ʼ��
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup(); //��WSAStartupһһ��Ӧ
	}
	BOOL InitSockEnv() {
		// �����׽��ֿ�
		WORD wVersionRequested;//����ָ����������� Winsock �汾
		WSADATA wsaData;    //���� Windows Sockets ʵ�ֵ���ϸ��Ϣ
		int err;
		//ͨ��MAKEWORD(1, 1)ָ���������� Winsock 1.1 �汾
		wVersionRequested = MAKEWORD(1, 1);
		// ��ʼ���׽��ֿ�
		//�������Ҫ��������ʹ�õ� Winsock �汾��һ�� `WSADATA` �ṹ��ָ�롣�������� `0` ��ʾ�ɹ���
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			return FALSE;
		}
		//���д����� Winsock ���ʵ�ʰ汾�Ƿ�������İ汾���ݡ�
		if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
		{
			//����汾�����ݣ������WSACleanup����ж��Winsock�⡣
			WSACleanup();
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  //delete�������ͷ��ڴ�ռ䣬��������������������
		}
	}

	static CServerSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};