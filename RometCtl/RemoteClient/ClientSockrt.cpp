#include "pch.h"
#include "ClientSockrt.h"

//CServerSocket server;
//��̬��Ա������ʼ������ľ�̬��Ա���������ڴ��������ʱ���ʼ���������ڳ����ȫ������ʼ����
CClientSockrt* CClientSockrt::m_instance = NULL;
CClientSockrt::CHelper CClientSockrt::m_helper;

CClientSockrt* pclient = CClientSockrt::getInstance();  //û��delete m_instance