// RometCtl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RometCtl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")

// 唯一的应用程序对象
//分支001
CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i{}; i < nSize; i++) {
        char buf[8]{ "" };
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

//创建磁盘分区的信息
int MakeDriverInfo() {
    std::string result;
    for (int i{1}; i <= 26; i++)
    {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) result += ',';//这里如果是中文符号会报错。(BYTE*)result.c_str()内容正确
            result += 'A' + i - 1;
        }
    }

    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    
    //Dump((BYTE*)&pack, pack.nLength + 2 + 4);  //逻辑有问题，里面有对象，不能对对象取地址，得不到对象里面的值。
    Dump((BYTE*)pack.Data(), pack.Size());
    
    //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size())); //bool Send(const CPacket& pack)不会报错
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <stdio.h>
#include <io.h>
//#include <list>

int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFPO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！！"));
        return -1;
    }
    //改变当前的工作目录到指定的路径
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFPO finfo;
        finfo.HasNext = FALSE;
        //lstFileInfos.push_back(finfo);
        //打包发送数据
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);

        OutputDebugString(_T("没有权限，访问目录！！"));
        return -2;
    }
    //用来存储文件属性的结构体，如文件名、时间戳、文件大小等
    _finddata_t fdata;
    //这个变量用来存储_findfirst函数的返回值，即一个搜索句柄，它标识了一个搜索状态。
    int hfind{};
    //_findfirst函数开始在当前目录下搜索匹配指定通配符（这里是"*"，意味着所有文件）的文件。
    //如果找到了，_findfirst会返回一个搜索句柄（一个非负整数），并将找到的第一个文件的信息填充到fdata结构体中
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件！！"));
        FILEINFPO finfo;
        finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    do 
    {
        FILEINFPO finfo; //目录结构体
        //判断是不是目录
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;

        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		//lstFileInfos.push_back(finfo);
		//打包发送数据
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind,&fdata));
    //_findnext函数使用之前由_findfirst获取的搜索句柄hfind，并尝试找到下一个文件。
    // 如果找到了，_findnext返回0并更新fdata结构体为新找到的文件的信息。
    FILEINFPO finfo;
    finfo.HasNext = FALSE;
	//打包发送数据
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    //用于执行一个程序、打开一个文件、打开一个文件夹，或启动一个关联应用程序
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    //发送应答包
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
    long long data{};
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb"); //二进制只读方式打开文件
    
    if (err != 0) {
        //应答包
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != 0) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile); // 取得文件流的读取位置,data就是文件长度。
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);

        char buffer[1024]{ "" };
        size_t rlen{};
        do
        {
            rlen = fread(buffer, 1, 1024, pFile);
            //发送文件
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    //文件结束时发送一个包
	CPacket pack(4, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}
int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags{};
        switch (mouse.nButton)
        {
        case 0://左键
            nFlags = 1;
            break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键
			nFlags = 4;
			break;
        case 3://没有按键
            nFlags = 8;
            break;
        }
        // 设置（移动）系统鼠标光标到指定的屏幕坐标
        if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//设置鼠标位置
        switch (mouse.nAction)
        {
		case 0://单击
			nFlags |= 0x10;
			break;
		case 1://双击
			nFlags |= 0x20;
			break;
		case 2://按下
			nFlags |= 0x40;
			break;
		case 3://放开
			nFlags |= 0x80;
			break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //执行完不会break，在一次进行单击，也就是双击
		case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		    break; 
        case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
        case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
        case 0x24://中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); 
        case 0x14://中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
		case 0x44://中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    return 0;
}

int SendScreen() {
    //声明了一个CImage对象，它是一个用于处理图像的类，提供了一系列图像操作的方法，如加载、保存和创建图像等
    CImage screen;//GDI,填句柄
    //获取整个屏幕的设备上下文,如果此值为 NULL， 则 GetDC 将检索整个屏幕的 DC。
    HDC hSreen = ::GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hSreen, BITSPIXEL);//位宽 24 ARGB 8888 32位
    int nWidth = GetDeviceCaps(hSreen, HORZRES);//宽
    int nHeight = GetDeviceCaps(hSreen, VERTRES);//高
    //根据屏幕的宽度、高度和位深度创建一个图像。
    screen.Create(nWidth, nHeight, nBitPerPixel);
    //使用BitBlt函数将屏幕内容拷贝到CImage对象中。注意，这里硬编码了拷贝区域的大小为1920x1020，这可能不匹配所有屏幕尺寸。
    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hSreen, 0, 0, SRCCOPY);
    //释放之前获取的屏幕DC。没有窗口
    ReleaseDC(NULL, hSreen);
    //分配全局内存
    HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMen == NULL)return -1;
    
    IStream* pStream = NULL;
    //创建基于全局内存的流
    HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
    if (ret == S_OK) {
        //保存图像到流中
        screen.Save(pStream, Gdiplus::ImageFormatTIFF);
        LARGE_INTEGER bg{};
        pStream->Seek(bg, STREAM_SEEK_SET, NULL); //将文件指针还原，以便打包发送
        //锁定全局内存并获取数据指针
        PBYTE pData = (PBYTE)GlobalLock(hMen);
        //获取数据大小
        SIZE_T nSize = GlobalSize(hMen);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        //解锁全局内存
        GlobalUnlock(hMen);
    }

    //screen.Save(_T("test2024.tiff"), Gdiplus::ImageFormatTIFF);  //保存到文件

//     for (int i{}; i < 10; i++) {
//         //测量保存PNG和JPEG格式图像所需的时间
//         DWORD tick = GetTickCount64();
//         //分别保存图像为PNG和JPEG格式。
//         screen.Save(_T("test2024.png"), Gdiplus::ImageFormatPNG);
//         TRACE("png %d\r\n", GetTickCount64() - tick);
//         tick = GetTickCount64();
// 		screen.Save(_T("test2024.jpg"), Gdiplus::ImageFormatJPEG);
// 		TRACE("ipg %d\r\n", GetTickCount64() - tick);
//     }
    //释放CImage对象内部使用的DC。
    screen.ReleaseDC(); //释放掉screen.GetDC()
    return 0;
}

#include "LockInfoDialog.h"
CLockInfoDialog dlg;
unsigned threadid{};

unsigned __stdcall threadLockDlg(void* arg) {
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
	//创建窗口,绑定ID为IDD_DIALOG_INFO的模板，没有父窗口设为NULL
	//NULL参数指示这个对话框没有父窗口，使其成为顶级窗口。
	dlg.Create(IDD_DIALOG_INFO, NULL);
	//SW_SHOW参数指示窗口应以其最近的大小和位置显示。
	dlg.ShowWindow(SW_SHOW);

	//遮蔽后台窗口
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	//GetSystemMetrics函数用于获取系统的各种配置信息和指标
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	//它用来获取主显示器的全屏高度，不包括任务栏的高度
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    rect.bottom = LONG(1.03 * rect.bottom);
	TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
	dlg.MoveWindow(rect);
	//设置窗口位置
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	//限制鼠标功能
	ShowCursor(false);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//隐藏任务栏
	//限制鼠标活动范围
	rect.left = 0;
	rect.top = 0;
	rect.right = 1;
	rect.bottom = 1;
	ClipCursor(rect);

	//消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		//负责翻译键盘消息（如虚拟键码转换为字符消息）
		TranslateMessage(&msg);
		//将消息分派给窗口
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			//08意味着输出的十六进制数将被填充到至少8位，不足部分用0填充，大写X表示以大写字母显示十六进制数。
			//wParam通常包含了被按下的键的虚拟键码，而lParam包含了有关按键事件的额外信息，如重复计数、扫描码等。
			TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x1B) {//按esc退出
				break;
			}
		}
	}
	ShowCursor(true);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    //结束线程
    _endthreadex(0);
    return 0;
}

int LockMachine() {
    //开启线程
    //m_hWnd是它的窗口句柄,NULL或INVALID_HANDLE_VALUE表示窗口尚未创建或创建失败。
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
        //_beginthread(threadLockDlg, 0, NULL);//开启线程
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid = %d\r\n", threadid);
    }
    //应答包
    CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine(){
    //dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x2711);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0x2711); //发送消息
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect() {
    CPacket pack(1981, NULL, 0);
	bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\r\n", ret);
	return 0;
}

int ExcuteCommand(int nCmd) {
    int ret{};
    switch (nCmd)
	{
	case 1://查看磁盘分区
        ret = MakeDriverInfo();
		break;
	case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
		break;
	case 3://打开文件
        ret = RunFile();
		break;
	case 4://下载文件
        ret = DownloadFile();
		break;
	case 5://鼠标操作
        ret = MouseEvent();
		break;
	case 6://发送屏幕内容 =》 发送屏幕的截图
        ret = SendScreen();
		break;
	case 7://锁机
        ret = LockMachine();
		break;
	case 8://解锁
        ret = UnlockMachine();
		break;
	case 1981:
		ret = TestConnect();
		break;
	}
    return ret;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            //套接字：socket bind listen accept read write close
            //linux可以直接创建，但是win需要套接字环境的初始化。 
           //单例模式，只创建了一个实例，返回的永远都是m_instance。
            CServerSocket* pserver = CServerSocket::getInstance();
            int count{ 0 };
			if (pserver->InitSocket() == false) {
				MessageBox(NULL, _T("网络初始化异常，未能初始化，请检查网络！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				exit(0);
			}
            while (CServerSocket::getInstance() != NULL) {
                if (pserver->AcceptClient() == false) {
                    if ((count++) > 3) {
						MessageBox(NULL, _T("多次无法正常接入用户，自动结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
					MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    
                }
                TRACE("AcceptClient return true\r\n");
                int ret = pserver->DealCommond();  //收包 解包 拿到命令
                TRACE("DealCommond ret %d\r\n", ret);
                //TODO:处理命令
                if (ret > 0) {
                    //不能这样写，pserver->GetPacket().sCmd会返回-1
                    //ret = ExcuteCommand(pserver->GetPacket().sCmd);
                    ret = ExcuteCommand(pserver->GetPacket().sCmd);
                    if (ret != 0) {
                        TRACE("执行命令失败：%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();  //采用短连接
                    TRACE("cosket is doned!\r\n");
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}