// RometCtl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RometCtl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "MyTool.h"
#include <conio.h>
#include "MyToolQueue.h"
#include <MSWSock.h>
#include "MyServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")

//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RometCtl.exe")
#define INVOKE_PATH _T("C:\\Users\\86199\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RometCtl.exe")

// 唯一的应用程序对象
CWinApp theApp;
using namespace std;

bool ChooseAutoInvoke(const CString& strPath) {
    //开机启动的时候，程序的权限是跟随启动用户的
    //如果两者权限不一致，则会导致程序启动失败
    //开机h启动对环境变量有影响，如果以来dll（动态库），则可能启动失败
    //解决方法：
    //【复制这些dll到system32下面或者sysWOW64下面】
    //system32下面，多是64为程序 sysWOW64下面多是32为程序
    //【使用静态库，而非动态库】
    TCHAR wcsSystem[MAX_PATH] = _T("");
    if (PathFileExists(strPath)) {
        return true;
    }
    
    CString strInfo = _T("改程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监视的状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
		CMyTool::WriteStartupDir(strPath);
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

void iocp();

int main()
{
    if (!CMyTool::Init()) return 1;
    iocp();
// 	if (CMyTool::IsAdmin()) {
//         if (!CMyTool::Init()) return 1;
// 		//printf("current is run as administrator!\r\n");//命令行输出屏蔽掉了，不能printf
// 		OutputDebugString(L"current is run as administrator!\r\n");
// 		MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
// 		// TODO: 在此处为应用程序的行为编写代码。
// 		if (ChooseAutoInvoke(INVOKE_PATH)) {
// 			CCommand cmd;
// 			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);//为什么还要取地址？？
// 			switch (ret)
// 			{
// 			case -1:
// 				MessageBox(NULL, _T("网络初始化异常，未能初始化，请检查网络！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
// 				break;
// 			case -2:
// 				MessageBox(NULL, _T("多次无法正常接入用户，自动结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
// 				break;
// 			}
// 		}
// 	}
// 	else {
// 		OutputDebugString(L"current is run as normal user!\r\n");
// 		if (CMyTool::RunAsAdmin() == false) {//会再次调用main函数
// 			CMyTool::ShowError();
// 			return 1;
// 		}
// 		MessageBox(NULL, _T("普通用户已变更为管理员"), _T("用户状态"), 0);
// 	}
    return 0;
}

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        memset(&m_buffer, 0, sizeof(m_buffer));
    }
};

void iocp() {
    CMyServer server;
    server.StartServic();
    getchar();
}