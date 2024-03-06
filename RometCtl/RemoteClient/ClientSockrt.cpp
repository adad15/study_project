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
	if (m_sock == INVALID_SOCKET) {
		/*if (InitSocket() == false) return false;*/
		_beginthread(&CClientSockrt::threadEntry, 0, this);
	}
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	m_lstSend.push_back(pack);//���б�������һ��Ԫ��
	WaitForSingleObject(pack.hEvent, INFINITE);//��ͬ��-1,�ȴ��¼�
	std::map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) {
		
		m_mapAck.erase(it);//ת�Ƶ�lstPacksӦ�������ȥ��
		return true;
	}
	return false;
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
			CPacket& head = m_lstSend.front();//��һ��Ԫ��
			if (Send(head) == false) {
				TRACE("����ʧ�ܣ�\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>>::iterator it;
			it = m_mapAck.find(head.hEvent);
			std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
			do {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
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
						}
					}
					TRACE("index is %d\r\n", index);
				}
				//��������ʧ��
				else if (length <= 0 && index <= 0) {
					CloseSocket();
					SetEvent(head.hEvent);//�ȵ��������ر�����֮����֪ͨ�������
				}
			} while (it0->second == false);
			m_lstSend.pop_front();
			InitSocket();
		}
	}
	CloseSocket();
}
