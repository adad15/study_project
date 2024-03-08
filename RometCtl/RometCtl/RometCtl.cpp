// RometCtl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RometCtl.h"
                                                                #include "ServerSocket.h"
#include "Command.h"

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

void WriteRegisterTable(const CString& strPath) {
	//1.建立软链接
	char sPath[MAX_PATH]{ "" };
	char sSys[MAX_PATH]{ "" };
	std::string strExe{ "\\RometCtl.exe " };
	GetCurrentDirectoryA(MAX_PATH, sPath);
	GetSystemDirectoryA(sSys, sizeof(sSys));
	//不用复制动态链接库
	std::string strCmd = "cmd /K mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
	int ret = system(strCmd.c_str());
	//2.建立注册表
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	HKEY hKey = NULL;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败!"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}
	ret = RegSetValueEx(hKey, _T("RometCtl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败!"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}
	RegCloseKey(hKey);
}

void WriteStartupDir(const CString& strPath) {
    CString strCmd = GetCommandLine();//在这行代码中，strCmd（经过双引号删除处理后）被用作源文件路径。
    strCmd.Replace(_T("\""), _T(""));
    //CopyFile是另一个Windows API函数，用于复制文件。它接受三个参数：源文件路径（要复制的文件）、
    //目标文件路径（复制到的位置）、以及一个布尔值，指示在目标位置已存在同名文件时是否覆盖它。
    BOOL ret = CopyFile(strCmd, strPath, FALSE);//false 表示文件存在就会被覆盖
    //fopen CFile system(copy) CopyFile OpenFile
    if (ret == FALSE) {
		MessageBox(NULL, _T("复制文件失败！是否权限不足？\r\n程序启动失败!"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
    }
}

void ChooseAutoInvoke() {
    //开机启动的时候，程序的权限是跟随启动用户的
    //如果两者权限不一致，则会导致程序启动失败
    //开机h启动对环境变量有影响，如果以来dll（动态库），则可能启动失败
    //解决方法：
    //【复制这些dll到system32下面或者sysWOW64下面】
    //system32下面，多是64为程序 sysWOW64下面多是32为程序
    //【使用静态库，而非动态库】
    TCHAR wcsSystem[MAX_PATH] = _T("");
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RometCtl.exe"));
    CString strPath = _T("C:\\Users\\asd\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RometCtl.exe");
    if (PathFileExists(strPath)) {
        return;
    }
    
    CString strInfo = _T("改程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监视的状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        WriteStartupDir(strPath);
    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
    return;
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
            CCommand cmd;
            // TODO: 在此处为应用程序的行为编写代码。
            ChooseAutoInvoke();
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);//为什么还要取地址？？
            switch (ret)
            {
            case -1:
				MessageBox(NULL, _T("网络初始化异常，未能初始化，请检查网络！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
            case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，自动结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
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