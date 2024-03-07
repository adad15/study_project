#include "pch.h"
#include "ClientSockrt.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC>CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct{
			UINT nMsg;
			MSGFUNC func;
		}MsgFuncs[] = {
			/*{WM_SNED_PACK,&CClientController::OnSendPack },
			{WM_SEND_DATA,&CClientController::OnSendData },*/
			{WM_SHOW_STATUS,&CClientController::OnShowStatus },
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher },
			{(UINT) - 1,NULL}
		};

		for (int i{}; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	//创建m_remoteDlg窗口，后面OnShowStatus函数会显示该窗口
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);//Create函数的第二个参数，它指定了对话框的父窗口
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;

	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)return -2;
	MSGINFO info(msg);
	//向m_nThreadID线程发送消息
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}

//消息响应机制下的SendCommandPacket函数，异步机制
bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
	//发送封装好的数据包
	CClientSockrt* pClient = CClientSockrt::getInstance();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);//plstPacks为输出参数
	/*if (!ret) {
		Sleep(30);
		//第二次的时候会执行，是因为SendPacket里面调完_beginthreadex后，线程创建了，但是线程函数没有立刻被执行
		//资源线程号已经被分配好了，线程的内存空间已经被分配好了，只是目前CPU还没有给它分配时间去执行
		//所以后面PostThreadMessage会失败（只有线程函数调用::GetMessage才可以接受消息）
		//所以主线程休眠30毫秒，子线程有机会拿到CPU的资源
		//可以通过事件去处理
		ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);//plstPacks为输出参数
	}*/
	return ret;
}



//事件响应机制下的SendCommandPacket函数，同步机制
// 
// int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData,
// 	size_t nLength, std::list<CPacket>* plstPacks)//plstPacks = null 表示不关心应答
// {
// 	TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
// 	//发送封装好的数据包
// 	CClientSockrt* pClient = CClientSockrt::getInstance();
// 	/*if (pClient->InitSocket() == false) return false;*/
// 	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
// 	
// 	//不应该直接发送，而是投入队列。
// 	std::list<CPacket>lstPacks;//应答结果的包
// 	if (plstPacks == NULL)
// 		plstPacks = &lstPacks;
// 	pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent), *plstPacks, bAutoClose);//plstPacks为输出参数
// 	CloseHandle(hEvent);//回收事件句柄，防止资源耗尽
// 	//接受服务端发送的应答包，并解包
// 	if (plstPacks->size() > 0) {
// 		TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
// 		return plstPacks->front().sCmd;
// 	}
// 	TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
// 	return -1;
// }

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, "*",
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);
	// DoModal()方法显示对话框，并在用户关闭对话框时返回。如果用户点击“保存”按钮，DoModal()返回IDOK，
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;//拿到远程地址
		m_strLocal = dlg.GetPathName();//本地保存地址
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			return -1;
		}
		//开启一个新线程,处理文件
		//第二个参数 0 指的是新线程的堆栈大小。当设置为 0 时，表示使用默认的堆栈大小。堆栈大小可以影响到线程能够使用的局部变量的数量和大小。
		//第三个参数 this 指向当前的 CRemoteClientDlg 实例。这个参数被传递给 threadEntryForDownFile 函数，允许该函数访问类的成员变量和函数。
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//主线程弹出子窗口
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {//判断线程是的正确启动，线程启动时等待时间为0，必然等待超时
		//	return -1;
		//}

		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);//消息机制
		m_statusDlg.BeginWaitCursor();//光标设置为等待沙漏的状态
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
		//显示窗口
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		//SetActiveWindow API把窗口激活
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClose = false;
	m_hThreadWatch= (HANDLE)_beginthread(&CClientController::threadEntryForWatchData, 0, this);
	m_watchDlg.DoModal();//主线程阻塞在这里，所以只会开一个threadEntryForWatchData线程函数
	m_isClose = true;
	//这行代码等待之前创建的监控线程结束。WaitForSingleObject是一个同步函数，用于等待对象（这里是线程）进入信号状态或者超时。
	//这里的500表示超时时间，单位是毫秒。如果线程在500毫秒内结束，函数将返回；
	//如果500毫秒内线程没有结束，函数也会返回，但线程可能仍然在运行。
	WaitForSingleObject(m_hThreadWatch, 500);
}
//事件机制下的threadWatchScreen处理函数
// 
// void CClientController::threadWatchScreen()
// {
// 	Sleep(50);
// 	while (!m_isClose) {
// 		if (m_watchDlg.isFull() == false) {
// 			std::list<CPacket>lstPacks;
// 			int ret = SendCommandPacket(6, true, NULL, 0, &lstPacks);
// 			if (ret == 6) {
// 				if ((CMyTool::Bytes2Image(m_watchDlg.GetImage(), lstPacks.front().strData)) == 0){
// 				//if (GetImage(m_remoteDlg.GetImage()) == 0) {
// 					m_watchDlg.SetImageStatus(true);
// 					TRACE("成功设置图片 %08X\r\n", (HBITMAP)m_watchDlg.GetImage());
// 					TRACE("和校验：%04X\r\n", lstPacks.front().sSum);
// 				}
// 				else {
// 					TRACE("获取图片失败！ret = %d\r\n", ret);
// 				}
// 			}
// 			else {
// 				TRACE("获取图片失败！ret = %d\r\n", ret);
// 			}
// 		}
// 		Sleep(1);
// 	}
// 	TRACE("thread end %d\r\n", m_isClose);
// }

//消息机制下的threadWatchScreen处理函数
void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClose) {
		if (m_watchDlg.isFull() == false) {
			//图片发送不能太勤，服务端是单线程，一个命令一个命令的去相应
			//延时太严重
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}//保证每次间隔200毫秒发送
			nTick = GetTickCount64();
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO 添加消息响应函数WM_SEND_PACK_ACK
			//TODO 控制发送频率
			if (ret == 1) {
				TRACE("成功发送请求图片命令\r\n");
			}
			else {
				TRACE("获取图片失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
	TRACE("thread end %d\r\n", m_isClose);
}

void CClientController::threadEntryForWatchData(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL) {
		AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSockrt* pClient = CClientSockrt::getInstance();
	do 
	{
		//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength()); 
		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);//消息机制
		
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();//得到第一个数据包储存的文件长度
		if (nLength == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件！！！");
			break;
		}
		//开始正式读取文件内容
		long long nCount{};
		while (nCount < nLength)
		{
			//继续从服务器接受数据，然后从缓冲区解一个包
			ret = pClient->DealCommond();
			if (ret < 0) {
				AfxMessageBox("传输失败！！");
				TRACE("传输失败，ret = %d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
	m_remoteDlg.LoadFileInfo();//重新显示
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	//访问真正的线程函数
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		//TODO 处理消息
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;  //就是PostThreadMessage里面的info
			HANDLE hEvent = (HANDLE)msg.lParam;    //就是PostThreadMessage里面的hEvent
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSockrt* pClient = CClientSockrt::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket);
//}
//
//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSockrt* pClient = CClientSockrt::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, (int)lParam);
//}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}