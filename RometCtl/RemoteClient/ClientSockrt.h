#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#define BUFFER_SIZE 4096

#pragma pack(push)//保存字节对齐的状态
#pragma pack(1)

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	//打包数据
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFE;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		size_t a = strData.size();
		for (size_t j{}; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	//解析数据
	CPacket(const BYTE* pData, size_t& nSize) {
		//找包头
		size_t i{};
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFE) {
				sHead = *(WORD*)(pData + i);
				i += 2;  //i向后移动两字节，把包头部分除去
				break;
			}
			if (i + 4 + 2 + 2 > nSize) { //包数据可能不全，或者包头未能全部接收到
				nSize = 0;
				return;
			}
			nLength = *(DWORD*)(pData + i); i += 4; //i向后移动4字节，除去长度数据
			if (nLength + i > nSize) { //包未完全接收到，就返回解析失败
				nSize = 0;
				return;
			}
			sCmd = *(WORD*)(pData + i); i += 2; //i向后移动2字节，除去命令长度
			if (nLength > 4) {
				strData.resize(nLength - 2 - 2); //调整字符串的长度
				memcpy((void*)strData.c_str(), pData + i, nLength - 4); //复制字符串
				i += nLength - 4;
			}
			sSum = *(WORD*)(pData + i); i += 2;
			WORD sum{};
			for (size_t j{}; j < strData.size(); j++) {
				sum += BYTE(strData[j]) & 0xFF;
			}
			if (sum == sSum) {
				nSize = i; //不要忘记包头前面可能残余数据,一起销毁掉。
				//nSize = nLength + 2 + 4; //包头部分+长度部分
				return;
			}
			nSize = 0;
		}
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;   //实现连等
	}
	int Size() {//包数据的大小
		return nLength + 2 + 4;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str();
	}
public:
	WORD sHead;           //包头 0xFEFE
	DWORD nLength;        //包长度（从控制命令开始，到和校验结束）
	WORD sCmd;            //控制命令
	std::string strData;  //包数据
	WORD sSum;            //和校验，只检验包数据的长度
	std::string strOut;   //整个包的数据
};
#pragma pack(pop)//还原字节对齐的状态

typedef struct MouseEvent
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //点击、移动、双击
	WORD nButton; //左键、中键
	POINT ptXY;
}MOUSEEV, * PMOUSEEV;

//connect函数错误码处理函数
std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	//把错误码进行格式化
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

class CClientSockrt
{
public:
	static CClientSockrt* getInstance() {
		//静态成员函数没有this指针，无法直接访问私用成员变量
		if (m_instance == NULL)
		{
			m_instance = new CClientSockrt();
		}

		return m_instance;
	}
	bool InitSocket(const std::string& strIPAddress) {
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		//使用传递的ip地址
		serv_addr.sin_addr.S_un.S_addr = inet_addr(strIPAddress.c_str());
		serv_addr.sin_port = htons(9527);
		if (serv_addr.sin_addr.S_un.S_addr == INADDR_NONE) {
			AfxMessageBox("指定的ip地址，不存在！");
			return false;
		}

		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			//MessageBox是win32的，可以使用AfxMessageBox（MFC的）
			AfxMessageBox("连接失败！");
			//WSAGetLastError:返回错误号
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}
	
	int DealCommond() {
		if (m_sock == -1) return false;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index{};
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index;
			//调用重载等号运算符，创建匿名对象
			//如果有多个包合在一起，调用这个构造函数len的值会变，第一个包头前的废数据+第一个数据包长度
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				//读取完第一个数据包后，清楚这个数据包前面的内容，把后面的数据像前提。
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) { //不能常量引用，Data函数改变路pack的缓冲区
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}
	//将命令2解包后的类中的数据向外传递
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

private:
	SOCKET m_sock;
	CPacket m_packet;  //在CPacket类中必须要有复制构造函数
	CClientSockrt& operator=(const CClientSockrt& ss) {}
	CClientSockrt(const CClientSockrt& ss) {
		m_sock = ss.m_sock;
	}
	CClientSockrt() {
		//初始化套接字环境
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		//套接字变量初始化
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CClientSockrt() {
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
			CClientSockrt* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  //delete不仅仅释放内存空间，还会调用类的析构函数。
		}
	}

	static CClientSockrt* m_instance;
	//用来实现单例的初始化和析构
	class CHelper {
	public:
		CHelper() {
			CClientSockrt::getInstance();
		}
		~CHelper() {
			CClientSockrt::releaseInstance();
		}
	};
	static CHelper m_helper;
};

