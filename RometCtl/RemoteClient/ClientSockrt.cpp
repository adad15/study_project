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

//��Ϣ�����µ�SendPacket����
bool CClientSockrt::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	//PostThreadMessage�ǲ����Ͷ�ݵĽ���ģ�Ͷ��ʧ��Ҳ����������Ӧ��SendMessage��Ҫ��ϢӦ���Ժ��ȥ����
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;//����ʧ�ܵ�������������delete�����ͳɹ�������ڶ�Ӧ����Ϣ��Ӧ�����н��д���
	}
	return ret;
}

//�¼������µ�SendPacket����
// 
// bool CClientSockrt::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
// {
// 	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
// 		/*if (InitSocket() == false) return false;*/
// 		m_hThread = (HANDLE)_beginthread(&CClientSockrt::threadEntry, 0, this);//����ͬʱ�������߳�
// 		TRACE("start thread\r\n");
// 	}
// 	m_lock.lock();//����
// 	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
// 	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
// 	m_lstSend.push_back(pack);//���б�������һ��Ԫ��
// 	m_lock.unlock();//����
// 	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
// 	WaitForSingleObject(pack.hEvent, INFINITE);//��ͬ��-1,�ȴ��¼�
// 	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
// 	std::map<HANDLE, std::list<CPacket>&>::iterator it;
// 	it = m_mapAck.find(pack.hEvent);
// 	if (it != m_mapAck.end()) {
// 		m_lock.lock();//����
// 		m_mapAck.erase(it);//ת�Ƶ�lstPacksӦ�������ȥ��
// 		m_lock.unlock();//����
// 		return true;
// 	}
// 	return false;
// }

//�ص�����
void CClientSockrt::SendPack(UINT nMsg, WPARAM wParam/*��Ϣ�ṹ��*/, LPARAM lParam/*���ھ��*/)
//����һ����Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�ģʽ��  �ص���Ϣ�����ݽṹ��HWND MESSAGE��
{
	//�����߳�֮�䷢�����ݣ���÷���new�����ı����������Ǿֲ���������Ϊ�ֲ������ں��������������
	//��ǰ��wParam delete��
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam; //���ھ��

	if (InitSocket() == true) {
		
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index{};
			std::string strBuffer;
			strBuffer.reserve(BUFFER_SIZE);
			//���Զ���������ʹ��new����һЩ
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					//���
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE) {//���ǲ���������CSM_AUTOCLOSE�Զ��رգ����þ�ִ�����������
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
				}
				else {//TODO �Է��ر����׽��֣����������豸�쳣
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else {
			CloseSocket();
			//������ֹ����
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		//TODO ������
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);

	}
}

//void CClientSockrt::threadEntry(void* arg)//�¼������̺߳���
unsigned CClientSockrt::threadEntry(void* arg)//��Ϣ�����̺߳���
{
	CClientSockrt* thiz = (CClientSockrt*)arg;
	//thiz->threadFunc();  //�¼������̺߳���
	thiz->threadFunc2();   //��Ϣ�����̺߳���
	_endthreadex(0);
	return 0;
}

//�¼������µ��̺߳���
/*
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
}*/

void CClientSockrt::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message :%08X\r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			//ͨ�������ҵ�ֵ
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
