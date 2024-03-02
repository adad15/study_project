#pragma once
#include "ClientSockrt.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "MyTool.h"
#include <map>

#define WM_SNED_PACK (WM_USER+1)  //发送包数据
#define  WM_SEND_DATA (WM_USER+2) //发送数据
#define  WM_SHOW_STATUS (WM_USER+3)//展示状态
#define  WM_SHOW_WATCH (WM_USER+4)//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
	//单例模式,获取全局唯一对象
	static CClientController* getInstance();
	//初始化函数
	int InitController();
	//启动函数
	int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);

	//把ClientSockrt类中的方法进行映射
	//更新网络服务器的地址
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
	//1 查看磁盘分区
	//2 查看指定目录下的文件
	//3 打开文件
	//4 下载文件
	//5 鼠标操作
	//6 发送屏幕内容
	//7 锁机
	//8 解锁
	//9 删除文件
	//返回值，是命令号，如果小于零则是错误
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
	int GetImage(CImage& image) {
		CClientSockrt* pClient = CClientSockrt::getInstance();
		return CMyTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	//两个线程函数
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
	//线程函数接口
	static unsigned __stdcall threadEntry(void* arg);
	//真正的线程函数
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
	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
	bool m_isClose;//监控是否关闭

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