#pragma once
#include "afxdialogex.h"
#ifndef WM_SEND_PACK_ACK
#define  WM_SEND_PACK_ACK (WM_USER+2)//发送包数据应答
#endif

// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif
public:
	CImage m_image;
	int m_nObjWidth;
	int m_nObjHeight;
	bool isFull() const {
		return m_isFull;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	
	DECLARE_MESSAGE_MAP()
public:
	CImage& GetImage() {
		return m_image;
	}
	CPoint UserPointtoRemoteScreemPoint(CPoint& point, bool isScreen = false);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	bool m_isFull;  //true 表示有缓存数据
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lPrarm);//响应函数
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnUnlock();
	afx_msg void OnBnClickedBtnLock();
};
