#pragma once
#include "ClientSockrt.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "MyTool.h"
#include <map>

#define WM_SNED_PACK (WM_USER+1)  //���Ͱ�����
#define  WM_SEND_DATA (WM_USER+2) //��������
#define  WM_SHOW_STATUS (WM_USER+3)//չʾ״̬
#define  WM_SHOW_WATCH (WM_USER+4)//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000) //�Զ�����Ϣ����

class CClientController
{
public:
	//����ģʽ,��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitController();
	//��������
	int Invoke(CWnd*& pMainWnd);
	//������Ϣ
	LRESULT SendMessage(MSG msg);

	//��ClientSockrt���еķ�������ӳ��
	//��������������ĵ�ַ
	void UpdateAddress(int nIP,int nPort){
		CClientSockrt::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSockrt::getInstance()->DealCommond();
	}
	void CloseSocket() {
		CClientSockrt::getInstance()->CloseSocket();
	}
	bool SendPackst(const CPacket& pack) {
		CClientSockrt* pClient = CClientSockrt::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}
	//1 �鿴���̷���
	//2 �鿴ָ��Ŀ¼�µ��ļ�
	//3 ���ļ�
	//4 �����ļ�
	//5 ������
	//6 ������Ļ����
	//7 ����
	//8 ����
	//9 ɾ���ļ�
	//����ֵ��������ţ����С�������Ǵ���
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
	int GetImage(CImage& image) {
		CClientSockrt* pClient = CClientSockrt::getInstance();
		return CMyTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	//�����̺߳���
	void threadWatchScreen();
	static void threadEntryForWatchData(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);

	CClientController():
	m_statusDlg(&m_remoteDlg),
	m_watchDlg(&m_remoteDlg)
	{
		m_isClose = true;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	//�̺߳����ӿ�
	static unsigned __stdcall threadEntry(void* arg);
	//�������̺߳���
	void threadFunc();
	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo {
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC>m_mapFunc;
	//std::map<UUID, MsgInfo>m_mapMessage;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	unsigned m_nThreadID;
	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;
	bool m_isClose;//����Ƿ�ر�

	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};