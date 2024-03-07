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

//消息机制下的SendPacket函数
bool CClientSockrt::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	//PostThreadMessage是不会管投递的结果的，投递失败也不会做出反应，SendMessage是要消息应答以后才去结束
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;//发送失败的情况在这里进行delete，发送成功的情况在对应的消息响应函数中进行处理。
	}
	return ret;
}

//事件机制下的SendPacket函数
// 
// bool CClientSockrt::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
// {
// 	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
// 		/*if (InitSocket() == false) return false;*/
// 		m_hThread = (HANDLE)_beginthread(&CClientSockrt::threadEntry, 0, this);//避免同时起两个线程
// 		TRACE("start thread\r\n");
// 	}
// 	m_lock.lock();//上锁
// 	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
// 	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
// 	m_lstSend.push_back(pack);//向列表后端增加一个元素
// 	m_lock.unlock();//解锁
// 	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
// 	WaitForSingleObject(pack.hEvent, INFINITE);//等同于-1,等待事件
// 	TRACE("cmd:%d event %08X thread id %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
// 	std::map<HANDLE, std::list<CPacket>&>::iterator it;
// 	it = m_mapAck.find(pack.hEvent);
// 	if (it != m_mapAck.end()) {
// 		m_lock.lock();//上锁
// 		m_mapAck.erase(it);//转移到lstPacks应答包里面去了
// 		m_lock.unlock();//解锁
// 		return true;
// 	}
// 	return false;
// }

//回调函数
void CClientSockrt::SendPack(UINT nMsg, WPARAM wParam/*消息结构体*/, LPARAM lParam/*窗口句柄*/)
//定义一个消息的数据结构（数据和数据长度，模式）  回调消息的数据结构（HWND MESSAGE）
{
	//两个线程之间发送数据，最好发送new出来的变量，而不是局部变量。因为局部变量在函数结束后会析构
	//提前把wParam delete掉
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam; //窗口句柄

	if (InitSocket() == true) {
		
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index{};
			std::string strBuffer;
			strBuffer.reserve(BUFFER_SIZE);
			//会自动析构，比使用new方便一些
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					//解包
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE) {//看是不是设置了CSM_AUTOCLOSE自动关闭，设置就执行括号里代码
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
				}
				else {//TODO 对方关闭了套接字，或者网络设备异常
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else {
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		//TODO 错误处理
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);

	}
}

//void CClientSockrt::threadEntry(void* arg)//事件机制线程函数
unsigned CClientSockrt::threadEntry(void* arg)//消息机制线程函数
{
	CClientSockrt* thiz = (CClientSockrt*)arg;
	//thiz->threadFunc();  //事件机制线程函数
	thiz->threadFunc2();   //消息机制线程函数
	_endthreadex(0);
	return 0;
}

//事件机制下的线程函数
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
			m_lock.lock();//上锁
			CPacket& head = m_lstSend.front();//第一个元素
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("发送失败！\r\n");
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
						size_t size = (size_t)index;//缓冲区的总长度
						CPacket pack((BYTE*)pBuffer, size);//解析数据
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
					//接受数据失败或者数据处理完成
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);//等到服务器关闭命令之后，再通知事情完成
						if (it0 != m_mapAutoClosed.end()) {
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
							m_mapAutoClosed.erase(it0);
						}
						else {
							TRACE("异常的情况，没有对应的pair\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();//上锁
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
			//通过键，找到值
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
