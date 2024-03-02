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
			{WM_SNED_PACK,&CClientController::OnSendPack },
			{WM_SEND_DATA,&CClientController::OnSendData },
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
	//����m_remoteDlg���ڣ�����OnShowStatus��������ʾ�ô���
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);//Create�����ĵڶ�����������ָ���˶Ի���ĸ�����
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
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
	WaitForSingleObject(hEvent, -1);
	return info.result;
}

int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	//���ͷ�װ�õ����ݰ�
	CClientSockrt* pClient = CClientSockrt::getInstance();
	if (pClient->InitSocket() == false) return false;
	pClient->Send(CPacket(nCmd, pData, nLength));
	//���ܷ���˷��͵�Ӧ����������
	int cmd = DealCommand();
 	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose)
		CloseSocket();
	return cmd;
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, "*",
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);
	// DoModal()������ʾ�Ի��򣬲����û��رնԻ���ʱ���ء�����û���������桱��ť��DoModal()����IDOK��
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;//�õ�Զ�̵�ַ
		m_strLocal = dlg.GetPathName();//���ر����ַ
		//����һ�����߳�,�����ļ�
		//�ڶ������� 0 ָ�������̵߳Ķ�ջ��С��������Ϊ 0 ʱ����ʾʹ��Ĭ�ϵĶ�ջ��С����ջ��С����Ӱ�쵽�߳��ܹ�ʹ�õľֲ������������ʹ�С��
		//���������� this ָ��ǰ�� CRemoteClientDlg ʵ����������������ݸ� threadEntryForDownFile ����������ú���������ĳ�Ա�����ͺ�����
		m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//���̵߳����Ӵ���
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {//�ж��߳��ǵ���ȷ�������߳�����ʱ�ȴ�ʱ��Ϊ0����Ȼ�ȴ���ʱ
			return -1;
		}

		m_statusDlg.BeginWaitCursor();//�������Ϊ�ȴ�ɳ©��״̬
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
		//��ʾ����
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		//SetActiveWindow API�Ѵ��ڼ���
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClose = false;
	m_hThreadWatch= (HANDLE)_beginthread(&CClientController::threadEntryForWatchData, 0, this);
	m_watchDlg.DoModal();
	m_isClose = true;
	//���д���ȴ�֮ǰ�����ļ���߳̽�����WaitForSingleObject��һ��ͬ�����������ڵȴ������������̣߳������ź�״̬���߳�ʱ��
	//�����500��ʾ��ʱʱ�䣬��λ�Ǻ��롣����߳���500�����ڽ��������������أ�
	//���500�������߳�û�н���������Ҳ�᷵�أ����߳̿�����Ȼ�����С�
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClose) {
		if (m_watchDlg.isFull() == false) {
			int ret = SendCommandPacket(6);
			if (GetImage(m_remoteDlg.GetImage()) == 0) {
				m_watchDlg.SetImageStatus(true);
			}
			else{
				TRACE("��ȡͼƬʧ�ܣ�ret = %d\r\n", ret);
			}
		}
	}
	Sleep(1);
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
		AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷�����������"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSockrt* pClient = CClientSockrt::getInstance();
	do 
	{
		int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
		
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();//�õ���һ�����ݰ�������ļ�����
		if (nLength == 0) {
			AfxMessageBox("�ļ�����Ϊ������޷���ȡ�ļ�������");
			break;
		}
		//��ʼ��ʽ��ȡ�ļ�����
		long long nCount{};
		while (nCount < nLength)
		{
			//�����ӷ������������ݣ�Ȼ��ӻ�������һ����
			ret = pClient->DealCommond();
			if (ret < 0) {
				AfxMessageBox("����ʧ�ܣ���");
				TRACE("����ʧ�ܣ�ret = %d\r\n", ret);
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
	m_remoteDlg.MessageBox(_T("������ɣ���"), _T("���"));
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
	//�����������̺߳���
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
		//TODO ������Ϣ
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;  //����PostThreadMessage�����info
			HANDLE hEvent = (HANDLE)msg.lParam;    //����PostThreadMessage�����hEvent
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

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSockrt* pClient = CClientSockrt::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	return pClient->Send(*pPacket);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSockrt* pClient = CClientSockrt::getInstance();
	char* pBuffer = (char*)wParam;
	return pClient->Send(pBuffer, (int)lParam);
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}