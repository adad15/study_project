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
	//��������
	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;
		//strerror(errno);//��׼C���Կ�
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM/*��ʽ����Ϣ*/ | FORMAT_MESSAGE_ALLOCATE_BUFFER/*�Զ�����buf*/,
			NULL, GetLastError()/*��ʾ��ǰ�̴߳�����*/, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)/*����*/,
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);//��ӡ����
		MessageBox(NULL, lpMessageBuf, _T("���ʹ���"), 0);
		LocalFree(lpMessageBuf);//�ͷ��ڴ�ռ�
	}
	//MFC����µ�����������
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			// TODO: ���Ĵ�������Է�����Ҫ
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		// ��ʼ�� MFC ����ʧ��ʱ��ʾ����
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
	//��ȡ��ǰ���̵�Ȩ��
	static bool IsAdmin() {
		//���ƣ�ʹ�þ������ʾ
		HANDLE hToken = NULL;
		//ֻ��ʾ��ǰ�̵߳Ĵ���
		//OpenProcessToken ����������̹����ķ������ơ�
		if (!OpenProcessToken(GetCurrentProcess()/*��ȡ��ǰ���̵ľ��*/, TOKEN_QUERY/*��ѯ*/, &hToken)) {
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len{};
		if (GetTokenInformation(hToken, TokenElevation/*��Ȩ��Ϣ*/, &eve, sizeof(eve), &len) == false) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;//����Ȩ�ͷ��ش����������������Ȩ�ľ�����
		}
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}
	//��ȡ����ԱȨ�ޣ�ʹ�ø�Ȩ�޴�������
	static bool RunAsAdmin() {
		HANDLE hToken = NULL;
		BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("��¼����"), _T("�������"), 0);
			return false;
		}
		OutputDebugString(L"Logon administrator success!");
		STARTUPINFO si{};
		PROCESS_INFORMATION pi{};
		TCHAR sPath[MAX_PATH]{ _T("") };
		GetModuleFileName(NULL, sPath, MAX_PATH);
		//�����ӽ���
		//ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		ret = CreateProcessWithLogonW(_T("administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		CloseHandle(hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, sPath, _T("�������"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);//���޵ȴ����̽���
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
	//��ӳ��������ļ�����ʵ��������
	static void WriteStartupDir(const CString& strPath) {
		CString strCmd = GetCommandLine();//�����д����У�strCmd������˫����ɾ������󣩱�����Դ�ļ�·����
		strCmd.Replace(_T("\""), _T(""));
		//CopyFile����һ��Windows API���������ڸ����ļ�������������������Դ�ļ�·����Ҫ���Ƶ��ļ�����
		//Ŀ���ļ�·�������Ƶ���λ�ã����Լ�һ������ֵ��ָʾ��Ŀ��λ���Ѵ���ͬ���ļ�ʱ�Ƿ񸲸�����
		BOOL ret = CopyFile(strCmd, strPath, FALSE);//false ��ʾ�ļ����ھͻᱻ����
		//fopen CFile system(copy) CopyFile OpenFile
		if (ret == FALSE) {
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��!"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
		}
	}
	//���ע���ķ�ʽ������
	static void WriteRegisterTable(const CString& strPath) {
		//1.����������
		char sPath[MAX_PATH]{ "" };
		char sSys[MAX_PATH]{ "" };
		std::string strExe{ "\\RometCtl.exe " };
		GetCurrentDirectoryA(MAX_PATH, sPath);
		GetSystemDirectoryA(sSys, sizeof(sSys));
		//���ø��ƶ�̬���ӿ�
		std::string strCmd = "cmd /K mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
		int ret = system(strCmd.c_str());
		//2.����ע���
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��!"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
		}
		ret = RegSetValueEx(hKey, _T("RometCtl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��!"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
		}
		RegCloseKey(hKey);
	}
};