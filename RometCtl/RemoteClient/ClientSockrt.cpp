#include "pch.h"
#include "ClientSockrt.h"

//CServerSocket server;
//静态成员变量初始化，类的静态成员变量不会在创建对象的时候初始化，必须在程序的全局区初始化。
CClientSockrt* CClientSockrt::m_instance = NULL;
CClientSockrt::CHelper CClientSockrt::m_helper;

CClientSockrt* pclient = CClientSockrt::getInstance();  //没有delete m_instance