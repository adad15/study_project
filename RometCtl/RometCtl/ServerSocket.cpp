#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
//��̬��Ա������ʼ������ľ�̬��Ա���������ڴ��������ʱ���ʼ���������ڳ����ȫ������ʼ����
CServerSocket* CServerSocket::m_instance = NULL; 
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();  //û��delete m_instance