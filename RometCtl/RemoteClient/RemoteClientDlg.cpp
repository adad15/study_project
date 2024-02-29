
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
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

//封装一个网络连接函数
int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	//取控件上的值
	UpdateData();

	// 获取套接字的实例
	CClientSockrt* pClient = CClientSockrt::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPos));
	if (!ret) {
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	//发送封装好的数据包
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret %d\r\n", ret);

	//接受服务端发送的应答包，并解包
	int cmd = pClient->DealCommond();
	TRACE("ack:%d\r\n", pClient->GetPacket().sCmd);
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose)
		pClient->CloseSocket();
	return cmd;
}

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
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket) //注册消息
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
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
	UpdateData(FALSE);

	//创建子窗口，并设置为隐藏
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;
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

		// 使图标在工作区矩形中居中
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
	//取控件上的值
	//UpdateData();

	// 获取套接字的实例
	//CClientSockrt* pClient = CClientSockrt::getInstance();
	//bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPos));
	//if (!ret) {
		//AfxMessageBox("网络初始化失败！");
		//return;
	//}
	//发送封装好的数据包
	//CPacket pack(1981, NULL, 0);
	//ret = pClient->Send(pack);
	//TRACE("Send ret %d\r\n", ret);

	//接受服务端发送的应答包，并解包
	//int cmd = pClient->DealCommond();
	//TRACE("ack:%d\r\n", pClient->GetPacket().sCmd);
	//TRACE("ack:%d\r\n", cmd);
	//pClient->CloseSocket();

	//把上面的代码封装成函数
	SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1); //连接服务器,拿到磁盘信息，并解包
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


void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	
	_endthread();
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();

	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
//=====================可能存在异步问题导致程序崩溃=================
	Sleep(50);//保证窗口先弹出
	//循环网络连接，直到连接成功
	CClientSockrt* pClient = NULL;
	do {
		pClient = CClientSockrt::getInstance();
	} while (pClient == NULL);

	while (!m_isClose) {//等价于while(true)
		if (m_isFull == false) {
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6) {
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				//TODO: 存入CImage
				HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0); //分配全局内存
				if (hMen == NULL) {
					TRACE("内存不足了!");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				//创建基于全局内存的流
				HRESULT hRet = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
				if (hRet == S_OK) {
					ULONG length{};
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg{};
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);//将文件指针还原
					if ((HBITMAP)m_image != NULL) m_image.Destroy();
					m_image.Load(pStream);
					m_isFull = true;
				}
			}
			else {
				//加Sleep后则允许cpu处理其他进程
				Sleep(1);//如果send的时候，网络突然断掉，会死循环，cpu拉满

			}
		}
		else {
			Sleep(1);
		}
	}
}

void CRemoteClientDlg::threadDownFile()
{
	//这行代码获取当前在列表视图（m_List）中选中的项的索引
	//GetSelectionMark() 函数返回最后一次选中的项的索引。如果没有选中的项，通常会返回-1。
	int nListSelected = m_List.GetSelectionMark();
	//使用上一步获取的索引，这行代码获取列表视图中选中项的文本
	//GetItemText() 函数接受两个参数：项的索引和列的索引。这里，索引0代表第一列，这通常是文件或文件夹的名称。
	CString strFile = m_List.GetItemText(nListSelected, 0);

	//这个参数指定对话框是保存文件对话框（FALSE）还是打开文件对话框（TRUE）。这里FALSE表示它是一个保存文件对话框。
	//"*"：这个参数指定了默认的文件扩展名
	//strFile：这个参数提供了默认的文件名。如果strFile非空，对话框会使用这个文件名作为默认值。
	//this：这个参数指定了对话框的父窗口。在这个例子中，this指的是当前对象，意味着对话框的父窗口是创建它的窗口。
	CFileDialog dlg(FALSE, "*",
		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, this);
	// DoModal()方法显示对话框，并在用户关闭对话框时返回。如果用户点击“保存”按钮，DoModal()返回IDOK，
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			return;
		}

		//这行代码获取树视图（m_Tree）中当前选中的项
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", LPCSTR(strFile));

		CClientSockrt* pClient = CClientSockrt::getInstance();
		//发送接受数据包，解第一个包，储在在m_pack里（第一个包是长度）
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			//4 => 0100    (4 << 1) => 1000  (1000)|0 => 1000  (1000)&1 => 1000)&(0001) => 0
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败！！！");
				TRACE("执行下载失败：ret = %d\r\n", ret);
				break;
			}

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
		} while (0);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！！"), _T("完成"));
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

	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	//拿到解包好的文件结构体指针
	PFILEINFPO pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();

	CClientSockrt* pClient = CClientSockrt::getInstance();
	while (pInfp->HasNext)
	{
		if (pInfp->IsDirectory) {
			if (CString(pInfp->szFileName) == "." || CString(pInfp->szFileName) == "..") {
				//如果是这两个那么不加入到树形菜单中，剩下的代码不变
				int cmd = pClient->DealCommond();   //不断接受路径包
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
		
		int cmd = pClient->DealCommond();   //不断接受路径包
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfp = (PFILEINFPO)CClientSockrt::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

//文件删除后列表刷新函数
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	//复用LoadFileInfo里面的代码，删除tree相关的代码，因为没有改变tree
	//只删除文件夹就行了
	m_List.DeleteAllItems();

	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
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
	pClient->CloseSocket();
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
	//开启一个新线程,处理文件
	//第二个参数 0 指的是新线程的堆栈大小。当设置为 0 时，表示使用默认的堆栈大小。堆栈大小可以影响到线程能够使用的局部变量的数量和大小。
	//第三个参数 this 指向当前的 CRemoteClientDlg 实例。这个参数被传递给 threadEntryForDownFile 函数，允许该函数访问类的成员变量和函数。
	_beginthread(CRemoteClientDlg::threadEntryForDownFile,0, this);
	//主线程弹出子窗口
	BeginWaitCursor(); //光标设置为等待沙漏的状态
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	//显示窗口
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	//SetActiveWindow API把窗口激活
	m_dlgStatus.SetActiveWindow();
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
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
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！");
	}
}

//消息函数
LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret{};
	int cmd = wParam >> 1;
	switch (cmd) {
	case 4:
		{
			// int ret = SendCommandPacket(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
			CString strFile = (LPCSTR)lParam;
			//wParam右移一位
			ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		}
		break;
	case 5:
		{//鼠标操作
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
		}
	break;
	case 6:
	case 7:
	case 8:
	{
		ret = SendCommandPacket(cmd, wParam & 1);
	}
	break;
	default:
		ret = -1;
	}
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	m_isClose = false;
	CWatchDialog dlg(this);
	// TODO: 在此添加控件通知处理程序代码
	HANDLE hTread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
	m_isClose = true;
	//这行代码等待之前创建的监控线程结束。WaitForSingleObject是一个同步函数，用于等待对象（这里是线程）进入信号状态或者超时。
	// 这里的500表示超时时间，单位是毫秒。如果线程在500毫秒内结束，函数将返回；
	// 如果500毫秒内线程没有结束，函数也会返回，但线程可能仍然在运行。
	//WaitForSingleObject(hTread, 500);
}

// void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
// {
// 	// TODO: 在此添加消息处理程序代码和/或调用默认值
// 
// 	CDialogEx::OnTimer(nIDEvent);
// }
