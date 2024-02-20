#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		//静态成员函数没有this指针，无法直接访问私用成员变量
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
		
		if (listen(m_sock, 1) == -1) return false;//只允许一个控制端进行控制，长度设为1

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
			//TODO:处理命令

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
		//套接字变量初始化
		m_client = INVALID_SOCKET;  //这个宏就是-1.

		//初始化套接字环境
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		//套接字变量初始化
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup(); //和WSAStartup一一对应
	}
	BOOL InitSockEnv() {
		// 加载套接字库
		WORD wVersionRequested;//用于指定程序请求的 Winsock 版本
		WSADATA wsaData;    //接收 Windows Sockets 实现的详细信息
		int err;
		//通过MAKEWORD(1, 1)指定程序请求 Winsock 1.1 版本
		wVersionRequested = MAKEWORD(1, 1);
		// 初始化套接字库
		//这个函数要求传入期望使用的 Winsock 版本和一个 `WSADATA` 结构体指针。函数返回 `0` 表示成功。
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			return FALSE;
		}
		//这行代码检查 Winsock 库的实际版本是否与请求的版本兼容。
		if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
		{
			//如果版本不兼容，则调用WSACleanup函数卸载Winsock库。
			WSACleanup();
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  //delete不仅仅释放内存空间，还会调用类的析构函数。
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