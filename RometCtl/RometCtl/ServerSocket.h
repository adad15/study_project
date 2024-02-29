#pragma once
#include "pch.h"
#include "MyTool.h"
#include "framework.h"
#include <list>
#include "Packet.h"

#define BUFFER_SIZE 409600

typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstCPacket,CPacket& inPacket);//��������ָ��

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

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		//�׽��֣�socket bind listen accept read write close
		//linux����ֱ�Ӵ���������win��Ҫ�׽��ֻ����ĳ�ʼ���� 
		//����ģʽ��ֻ������һ��ʵ�������ص���Զ����m_instance��
		bool ret = InitSocket(port);
		if (ret == false) return -1;

		std::list<CPacket> lstPackets;

		m_callback = callback;
		m_arg = arg;

		int count{};
		while (true)
		{
			if (AcceptClient() == false) {
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommond();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet);
				while (lstPackets.size() > 0) {
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();//���ö�����
		}
		return 0;
	}
protected:
	bool InitSocket(short port) {
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		
		if (listen(m_sock, 1) == -1) return false;//ֻ����һ�����ƶ˽��п��ƣ�������Ϊ1

		return true;
	}

	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1) return false;
		//closesocket(m_client);
		return true;
	}
	int DealCommond(){
		if (m_client == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index{};  
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			//�������صȺ��������������������
			//����ж��������һ�𣬵���������캯��len��ֵ��䣬��һ����ͷǰ�ķ�����+��һ�����ݰ�����
			m_packet = CPacket((BYTE*)buffer, len); 
			if (len > 0) {
				//��ȡ���һ�����ݰ������������ݰ�ǰ������ݣ��Ѻ����������ǰ�ᡣ
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}
// 	CPacket& GetPacket() {
// 		return m_packet;
// 	}

	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) { //���ܳ������ã�Data�����ı�·pack�Ļ�����
		if (m_client == -1) return false;
		CMyTool::Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	//������2���������е��������⴫��
// 	bool GetFilePath(std::string& strPath) {
// 		if (((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) || (m_packet.sCmd == 9)) {
// 			strPath = m_packet.strData;
// 			return true;
// 		}
// 		return false;
// 	}
// 	bool GetMouseEvent(MOUSEEV& mouse) {
// 		if (m_packet.sCmd == 5) {
// 			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
// 			return true;
// 		}
// 		return false;
// 	}
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;  //��CPacket���б���Ҫ�и��ƹ��캯��
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
	//����ʵ�ֵ����ĳ�ʼ��������
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