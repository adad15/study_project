
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSockrt.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1) //发送数据包的消息

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
	// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	bool isFull() const {
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
private:
	CImage m_image; //缓存
	bool m_isFull;  //true表示有缓存数据
	bool m_isClose;//监控是否关闭
private:
	//监控线程函数
	static void threadEntryForWatchData(void* arg);//无this指针，不好调用成员方法
	void threadWatchData();//有this指针，访问成员变量很方便
	
	//线程函数,要声明为静态，不创建对象也能使用
	static void threadEntryForDownFile(void* arg);
	//把线程函数中的文件处理代码封装成函数
	void threadDownFile();
	
	void LoadFileInfo();
	void LoadFileCurrent();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildItem(HTREEITEM hTree);
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
// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus; //文件下载提示窗口

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPos;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	//写消息函数
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
};
