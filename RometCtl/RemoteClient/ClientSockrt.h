#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define BUFFER_SIZE 2048000
#define  WM_SEND_PACK (WM_USER+1)

#pragma pack(push)//保存字节对齐的状态
#pragma pack(1)

void Dump(BYTE* pData, size_t nSize);

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		hEvent = pack.hEvent;
	}
	//打包数据
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent) {
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
		this->hEvent = hEvent;
	}

	//解析数据
	CPacket(const BYTE* pData, size_t& nSize) :hEvent(INVALID_HANDLE_VALUE) {
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
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
			hEvent = pack.hEvent;
		}
		return *this;   //实现连等
	}
	int Size() {//包数据的大小
		return nLength + 2 + 4;
	}
	const char* Data(std::string& strOut) const{
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
	HANDLE hEvent;
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

//定义文件结构体
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录
	BOOL HasNext;//是否还有后续
	char szFileName[256];//文件名
}FILEINFPO, * PFILEINFPO;

//connect函数错误码处理函数
std::string GetErrorInfo(int wsaErrCode);

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
	bool InitSocket() {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		//套接字变量初始化
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		//使用传递的ip地址
		serv_addr.sin_addr.S_un.S_addr = htonl(m_nIP);
		serv_addr.sin_port = htons(m_nPort);
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
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();//TODO：多线程发送命令时可能会出现冲突
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index{};
		while (true)
		{
			//通常情况下，如果 send 或者 recv 函数返回 0，我们就认为对端关闭了连接，
			// 我们这端也关闭连接即可，这是实际开发时最常见的处理逻辑。
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0) && (int)index <= 0) {
				return -1;
			}
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len; //index 表示数组数据中的总长度
			len = index;
			//调用重载等号运算符，创建匿名对象
			//如果有多个包合在一起，调用这个构造函数len的值会变，第一个包头前的废数据+第一个数据包长度
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("command %d\r\n", m_packet.sCmd);
			if (len > 0) {
				//读取完第一个数据包后，清楚这个数据包前面的内容，把后面的数据像前提。
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
				TRACE("DealCommond m_packet.sCmd %d", m_packet.sCmd);
			}
		}
		return -1;
	}
	 
	CPacket& GetPacket() {
		return m_packet;
	}

	bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true); 
	
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
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(int nIP, int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:
	typedef void(CClientSockrt::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC>m_mapFunc;//消息映射表

	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::map<HANDLE, bool>m_mapAutoClosed;
	std::list<CPacket> m_lstSend;
	//list<CPacket>是指对方应答的一系列的数据包
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	int m_nIP;//地址
	int m_nPort;//端口
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;  //在CPacket类中必须要有复制构造函数
	CClientSockrt& operator=(const CClientSockrt& ss) {}
	CClientSockrt(const CClientSockrt& ss) {
		m_hThread = ss.m_hThread;
		m_mapFunc = ss.m_mapFunc;
		m_bAutoClose = ss.m_bAutoClose;
		m_sock = ss.m_sock;
		m_nPort = ss.m_nPort;
		m_nIP = ss.m_nIP;
	}
	CClientSockrt() :
		m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), 
		m_hThread(INVALID_HANDLE_VALUE) {
		struct {
			UINT message;
			MSGFUNC func;
		}funcs[] = {
			{WM_SEND_PACK,&CClientSockrt::SendPack},
			{0,NULL}
		};
		for (int i{}; funcs[i].message != 0; i++) {
			if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
				TRACE("插入失败，消息值：%d 函数值：%08X 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
			}
		}

		//初始化套接字环境
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}

		//缓冲区初始化
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSockrt() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup(); //和WSAStartup一一对应
	}
	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& pack) { //不能常量引用，Data函数改变路pack的缓冲区
		TRACE("m_sock = %d\r\n", m_sock);
		if (m_sock == -1) return false;
		std::string strout;
		pack.Data(strout);
		return send(m_sock, strout.c_str(), strout.size(), 0) > 0;
	}
	//回调函数
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	//线程函数
	static void threadEntry(void* arg);
	//事件处理机制
	void threadFunc();
	//消息处理机制，和事件处理机制相比，动态加载
	//单线程，不会进行数据同步
	void threadFunc2();

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