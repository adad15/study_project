// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPointtoRemoteScreemPoint(CPoint& point, bool isScreen)
{//800 450
	CRect clientRect;//用于表示一个矩形区域
	//如果不是全局坐标
	if (!isScreen) {
		ClientToScreen(&point);//转换为相对屏幕左上角的坐标（屏幕的绝对坐标）
	}
	m_picture.ScreenToClient(&point);//转换为客户区域坐标（相对m_picture控件左上角的坐标）
	TRACE("x=%d y=%d\r\n", point.x, point.y);
	//本地坐标，到远程坐标
	m_picture.GetWindowRect(clientRect); //获取 m_picture 控件的屏幕坐标
	TRACE("x=%d y=%d\r\n", clientRect.Width(), clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / (clientRect.Height()+30));
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_isFull = false;
	//0是事件的ID，50是毫秒，最后参数表示时间回调函数，默认为OnTimer
	//SetTimer(0, 45, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

//更新屏幕
void CWatchDialog::OnTimer(UINT_PTR nIDEvent){

	//// 取数据,这里检查定时器的标识符是否为0
	//if (nIDEvent == 0) {
	//	//返回的父窗口对象指针
	//	CClientController* pParent = CClientController::getInstance(); //类型转换，从父类到子类
	//	if (m_isFull) {
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);

	//		m_nObjWidth = m_image.GetWidth();
	//		m_nObjHeight = m_image.GetHeight();
	//
	//		//得到picture contol的Dc
	//		//通过GetImage()方法获取父窗口中的图像
	//		//使用BitBlt函数将该图像绘制到m_picture（一个界面控件）的设备上下文（DC）中
	//		//SRCCOPY是复制的模式，表示进行直接复制。
	//		//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
	//		m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(),
	//			0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
	//		//调用GetImage()返回的图像对象的Destroy()方法，可能是为了释放与这个图像相关的资源。
	//		m_image.Destroy();
	//		//把m_isFull设置为false。
	//		m_isFull = false;
	//	}
	//}
	CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lPrarm)
{
	if (lPrarm == -1 || lPrarm == -2)//错误处理
	{

	}
	else if (lPrarm == 1) {//对方关闭了套接字

	}
	else{
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam; //1.只这样做还是会有内存泄漏，因为这只是收到的情况，还有没有收到数据包的情况
			//2.还是会内存泄漏，这是因为服务器应答时CWatchDialog可能就关闭了，这样就没人来接受消息，所以没有调用delete
			switch (head.sCmd)
			{
			case 6:
			{

				CMyTool::Bytes2Image(m_image, head.strData);
				//代替了OnTimer，收到一个图片就发送一个，就不需要计时器了。
				CRect rect;
				m_picture.GetWindowRect(rect);

				m_nObjWidth = m_image.GetWidth();
				m_nObjHeight = m_image.GetHeight();

				//得到picture contol的Dc
				//通过GetImage()方法获取父窗口中的图像
				//使用BitBlt函数将该图像绘制到m_picture（一个界面控件）的设备上下文（DC）中
				//SRCCOPY是复制的模式，表示进行直接复制。
				//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(),
					0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL);
				TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
				//调用GetImage()返回的图像对象的Destroy()方法，可能是为了释放与这个图像相关的资源。
				m_image.Destroy();
				//把m_isFull设置为false。
				m_isFull = false;
			}
				break;
			case 5:
				TRACE("远程端应答了鼠标操作");
				break;
			case 7:
			case 8:
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 1;//双击
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event)); //消息机制
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}

//客户坐标
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	//保证没有拿到屏幕数据时，不会操作鼠标。
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//弹起
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;//没有按键
		event.nAction = 0;//移动
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}

	CDialog::OnMouseMove(nFlags, point);
}

//全局坐标
void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		CPoint point;
		GetCursorPos(&point);//拿到鼠标位置
		//坐标转换
		CPoint remote = UserPointtoRemoteScreemPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));//消息机制
	}
}

void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	//CClientController::getInstance()->SendCommandPacket(8);
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);//消息机制
}

void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);//消息机制
}