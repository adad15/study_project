#include "pch.h"
#include "ClientSockrt.h"

//CServerSocket server;
//静态成员变量初始化，类的静态成员变量不会在创建对象的时候初始化，必须在程序的全局区初始化。
CClientSockrt* CClientSockrt::m_instance = NULL;
CClientSockrt::CHelper CClientSockrt::m_helper;

CClientSockrt* pclient = CClientSockrt::getInstance();  //没有delete m_instance

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