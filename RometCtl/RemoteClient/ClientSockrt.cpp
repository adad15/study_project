#include "pch.h"
#include "ClientSockrt.h"

//CServerSocket server;
//��̬��Ա������ʼ������ľ�̬��Ա���������ڴ��������ʱ���ʼ���������ڳ����ȫ������ʼ����
CClientSockrt* CClientSockrt::m_instance = NULL;
CClientSockrt::CHelper CClientSockrt::m_helper;

CClientSockrt* pclient = CClientSockrt::getInstance();  //û��delete m_instance

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	//�Ѵ�������и�ʽ��
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

bool CClientSockrt::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		/*if (InitSocket() == false) return false;*/
		m_hThread = (HANDLE)_beginthread(&CClientSockrt::threadEntry, 0, this);//����ͬʱ�������߳�
		TRACE("start thread\r\n");
	}
	m_lock.lock();//����
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	m_lstSend.push_back(pack);//���б�������һ��Ԫ��
	m_lock.unlock();//����
	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE);//��ͬ��-1,�ȴ��¼�
	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) {
		m_lock.lock();//����
		m_mapAck.erase(it);//ת�Ƶ�lstPacksӦ�������ȥ��
		m_lock.unlock();//����
		return true;
	}
	return false;
}

//�ص�����
void CClientSockrt::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//����һ����Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�ģʽ��  �ص���Ϣ�����ݽṹ��HWND MESSAGE��
{
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);
		if (ret > 0) {

		}
		else {
			CloseSocket();
			//������ֹ����
		}
	}
	else {
		//TODO ������
	}
}

void CClientSockrt::threadEntry(void* arg)
{
	CClientSockrt* thiz = (CClientSockrt*)arg;
	thiz->threadFunc();
}

void CClientSockrt::threadFunc()
{
// 	if (InitSocket() == false) {
// 		return;
// 	}
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index{};
	InitSocket();
	while (m_sock != INVALID_SOCKET){
		if (m_lstSend.size() > 0) {
			TRACE("lstSend size:%d\r\n", m_lstSend.size());
			m_lock.lock();//����
			CPacket& head = m_lstSend.front();//��һ��Ԫ��
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("����ʧ�ܣ�\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv %d %d\r\n", length, index);
					if (length > 0 || index > 0) {
						index += length;
						size_t size = (size_t)index;//���������ܳ���
						CPacket pack((BYTE*)pBuffer, size);//��������
						if (size > 0) {
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
						TRACE("index is %d\r\n", index);
					}
					//��������ʧ�ܻ������ݴ������
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);//�ȵ��������ر�����֮����֪ͨ�������
						if (it0 != m_mapAutoClosed.end()) {
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
							m_mapAutoClosed.erase(it0);
						}
						else {
							TRACE("�쳣�������û�ж�Ӧ��pair\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();//����
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();

			if (InitSocket() == false) {
				InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();
}

void CClientSockrt::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			//ͨ�������ҵ�ֵ
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
