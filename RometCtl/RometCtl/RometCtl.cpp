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
    CString strPath = _T("C:\\Users\\86199\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RometCtl.exe");
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

//错误处理函数
void ShowError() {
    LPWSTR lpMessageBuf = NULL;
    //strerror(errno);//标准C语言库
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM/*格式化消息*/ | FORMAT_MESSAGE_ALLOCATE_BUFFER/*自动分配buf*/,
        NULL, GetLastError()/*显示当前线程错误编号*/, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)/*语言*/,
        (LPWSTR)&lpMessageBuf, 0, NULL);
    OutputDebugString(lpMessageBuf);//打印错误
    MessageBox(NULL, lpMessageBuf, _T("发送错误"), 0);
    LocalFree(lpMessageBuf);//释放内存空间
}

//获取当前进程的权限
bool IsAdmin() {
    //令牌，使用句柄来表示
    HANDLE hToken = NULL;
    //只显示当前线程的错误
    //OpenProcessToken 函数打开与进程关联的访问令牌。
    if (!OpenProcessToken(GetCurrentProcess()/*获取当前进程的句柄*/, TOKEN_QUERY/*查询*/, &hToken)) {
        ShowError();
        return false;
    }
    TOKEN_ELEVATION eve;
    DWORD len{};
    if (GetTokenInformation(hToken, TokenElevation/*提权信息*/, &eve, sizeof(eve), &len) == false) {
        ShowError();
        return false;
    }
    CloseHandle(hToken);
    if (len == sizeof(eve)) {
        return eve.TokenIsElevated;//提了权就返回大于零的数，不是提权的就是零
    }
    printf("length of tokeninformation is %d\r\n", len);
    return false;
}

//获取管理员权限，使用该权限创建进程
void RunAsAdmin() {
    HANDLE hToken = NULL;
    BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
    if (!ret) {
        ShowError();
        MessageBox(NULL, _T("登录错误！"), _T("程序错误"), 0);
        exit(0);
    }
    OutputDebugString(L"Logon administrator success!");
    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    TCHAR sPath[MAX_PATH]{ _T("") };
    GetCurrentDirectory(MAX_PATH, sPath);
    CString strCmd = sPath;
    strCmd += _T("\\RometCtl.exe");
    //创建子进程
    //ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    ret = CreateProcessWithLogonW(_T("administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    CloseHandle(hToken);
    if (!ret) {
        ShowError();
        MessageBox(NULL, strCmd, _T("程序错误"), 0);
        exit(0);
    }
    WaitForSingleObject(pi.hProcess, INFINITE);//无限等待进程结束
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
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
			if (IsAdmin()) {
				//printf("current is run as administrator!\r\n");//命令行输出屏蔽掉了，不能printf
				OutputDebugString(L"current is run as administrator!\r\n");
				MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
			}
			else {
				OutputDebugString(L"current is run as normal user!\r\n");
				RunAsAdmin();
                MessageBox(NULL, _T("普通用户以变更管理员"), _T("用户状态"), 0);
				return nRetCode;
			}

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