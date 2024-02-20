#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
//静态成员变量初始化，类的静态成员变量不会在创建对象的时候初始化，必须在程序的全局区初始化。
CServerSocket* CServerSocket::m_instance = NULL; 
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();  //没有delete m_instance