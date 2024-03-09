#pragma once
class CMyTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
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
	//错误处理函数
	static void ShowError() {
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
	//MFC框架下的自启动代码
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			// TODO: 更改错误代码以符合需要
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
	//获取当前进程的权限
	static bool IsAdmin() {
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
	static bool RunAsAdmin() {
		HANDLE hToken = NULL;
		BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("登录错误！"), _T("程序错误"), 0);
			return false;
		}
		OutputDebugString(L"Logon administrator success!");
		STARTUPINFO si{};
		PROCESS_INFORMATION pi{};
		TCHAR sPath[MAX_PATH]{ _T("") };
		GetModuleFileName(NULL, sPath, MAX_PATH);
		//创建子进程
		//ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		ret = CreateProcessWithLogonW(_T("administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		CloseHandle(hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, sPath, _T("程序错误"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);//无限等待进程结束
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
	//添加程序到启动文件夹中实现自启动
	static void WriteStartupDir(const CString& strPath) {
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
	//添加注册表的方式自启动
	static void WriteRegisterTable(const CString& strPath) {
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
};