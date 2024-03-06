
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "ClientController.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPos(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPos);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

////封装一个网络连接函数
//int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
//{
//	//发送数据包,解包，关闭套接字
//	return CClientController::getInstance()->SendCommandPacket(nCmd, bAutoClose, pData, nLength);
//
//}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0xC0A83865;
	m_nPos = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPos));
	UpdateData(FALSE);

	//创建子窗口，并设置为隐藏
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中--*-0
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = CClientController::getInstance()->SendCommandPacket(1); //连接服务器,拿到磁盘信息，并解包
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}

	CClientSockrt* pClient = CClientSockrt::getInstance();
	std::string drivers = pClient->GetPacket().strData;  //得到解包后的命令信息
	std::string dr;
	//清理m_Tree控件
	m_Tree.DeleteAllItems();

	for (size_t i{}; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ":";
			//把信息添加到根目录，TVI_LAST表示追加的形式。
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST); //在目录节点后面添加空的子节点
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ":";
		HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		m_Tree.InsertItem(NULL, hTemp, TVI_LAST); //在目录节点后面添加空的子节点
	}
}

//获取树形结构（如目录树）中某个节点（HTREEITEM hTree）到根节点路径的函数
CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do 
	{
		//调用获取当前节点（hTree）的文本
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		//调用获取当前节点的父节点，并更新hTree以便在下次迭代中使用
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do 
	{
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != nullptr);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	//拿到当前指针的位置，全局坐标系
	GetCursorPos(&ptMouse);
	//转换成客户端坐标
	m_Tree.ScreenToClient(&ptMouse);
	//确定鼠标点击位置对应的树形控件（通常是CTreeCtrl类型）中的节点（item）
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) {//什么都没点击到
		return;
	}

	//看选中的有没有子节点，没有就是文件，直接返回
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return;
	//防止多次双击造成的无限扩张
	DeleteTreeChildItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);

	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	//拿到解包好的文件结构体指针
	PFILEINFPO pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();

	while (pInfp->HasNext)
	{
		if (pInfp->IsDirectory) {
			if (CString(pInfp->szFileName) == "." || CString(pInfp->szFileName) == "..") {
				//如果是这两个那么不加入到树形菜单中，剩下的代码不变
				int cmd = CClientController::getInstance()->DealCommand();   //不断接受路径包
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfp->szFileName, hTreeSelected, TVI_LAST);
			//如果是目录，在节点的后面加上一个空的节点
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pInfp->szFileName); //0是序号位置
		}
		
		int cmd = CClientController::getInstance()->DealCommand();   //不断接受路径包
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();
	}
	//CClientController::getInstance()->CloseSocket();
}

//文件删除后列表刷新函数
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	//复用LoadFileInfo里面的代码，删除tree相关的代码，因为没有改变tree
	//只删除文件夹就行了
	m_List.DeleteAllItems();

	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	//拿到解包好的文件结构体指针
	PFILEINFPO pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();

	CClientSockrt* pClient = CClientSockrt::getInstance();
	while (pInfp->HasNext)
	{
		if (!pInfp->IsDirectory) {
			m_List.InsertItem(0, pInfp->szFileName); //0是序号位置
		}

		int cmd = pClient->DealCommond();   //不断接受路径包
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();
	}
	//pClient->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	//拿到全局坐标
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	//转成屏幕坐标
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu; //创建一个菜单,此时只有一个子菜单
	menu.LoadMenu(IDR_MENU_RCLICK);
	//取第一个子菜单
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		//弹出菜单
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	//这行代码获取当前在列表视图（m_List）中选中的项的索引
	//GetSelectionMark() 函数返回最后一次选中的项的索引。如果没有选中的项，通常会返回-1。
	int nListSelected = m_List.GetSelectionMark();
	//使用上一步获取的索引，这行代码获取列表视图中选中项的文本
	//GetItemText() 函数接受两个参数：项的索引和列的索引。这里，索引0代表第一列，这通常是文件或文件夹的名称。
	CString strFile = m_List.GetItemText(nListSelected, 0);

	//这行代码获取树视图（m_Tree）中当前选中的项
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	TRACE("%s\r\n", LPCSTR(strFile));

	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！");
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！");
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	//取控件上的值
	UpdateData();
	// 获取套接字的实例
	CClientController* pController = CClientController::getInstance();
	//更新网络地址数据
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPos));
}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	//取控件上的值
	UpdateData();
	// 获取套接字的实例
	CClientController* pController = CClientController::getInstance();
	//更新网络地址数据
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPos));
}