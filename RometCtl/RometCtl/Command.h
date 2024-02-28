#pragma once
#include "resource.h"
#include <map>
#include <list>
#include <atlimage.h>
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include "Packet.h"
#include "MyTool.h"
#include "LockInfoDialog.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstCPacket,CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>&lstCPacket,CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status, lstCPacket, inPacket);
			if (ret!= 0) {
				TRACE("ִ������ʧ�ܣ�%d ret = %d\r\n", status, ret);
			}
		} 
		else {
			MessageBox(NULL, _T("�޷����������û����Զ�����"), _T("�����û�ʧ�ܣ�"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; //������ŵ����ܵ�ӳ��
	CLockInfoDialog dlg;
	unsigned threadid;

protected:
	static unsigned __stdcall threadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain() {
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
		//��������,��IDΪIDD_DIALOG_INFO��ģ�壬û�и�������ΪNULL
		//NULL����ָʾ����Ի���û�и����ڣ�ʹ���Ϊ�������ڡ�
		dlg.Create(IDD_DIALOG_INFO, NULL);
		//SW_SHOW����ָʾ����Ӧ��������Ĵ�С��λ����ʾ��
		dlg.ShowWindow(SW_SHOW);

		//�ڱκ�̨����
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		//GetSystemMetrics�������ڻ�ȡϵͳ�ĸ���������Ϣ��ָ��
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		//��������ȡ����ʾ����ȫ���߶ȣ��������������ĸ߶�
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom = LONG(1.1 * rect.bottom);
		TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
		dlg.MoveWindow(rect);

		//���������Ļ
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect reText;
			pText->GetWindowRect(reText);
			int nWidth = reText.Width();
			int x = (rect.right - nWidth) / 2;
			int nHeight = reText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, reText.Width(), reText.Height());
		}

		//���ô���λ��
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//������깦��
		ShowCursor(false);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//����������
		//���������Χ
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);

		//��Ϣѭ��
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			//�����������Ϣ�����������ת��Ϊ�ַ���Ϣ��
			TranslateMessage(&msg);
			//����Ϣ���ɸ�����
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				//08��ζ�������ʮ��������������䵽����8λ�����㲿����0��䣬��дX��ʾ�Դ�д��ĸ��ʾʮ����������
				//wParamͨ�������˱����µļ���������룬��lParam�������йذ����¼��Ķ�����Ϣ�����ظ�������ɨ����ȡ�
				TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x1B) {//��esc�˳�
					break;
				}
			}
		}
		//�ָ���귶Χ
		ClipCursor(NULL);
		//�ָ����
		ShowCursor(true);
		//�ָ�������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		//���ٴ���
		dlg.DestroyWindow();
		//�����߳�
	}

	//�������̷�������Ϣ
	int MakeDriverInfo(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string result;
		for (int i{ 1 }; i <= 26; i++)
		{
			if (_chdrive(i) == 0) {
				if (result.size() > 0) result += ',';//������������ķ��Żᱨ��(BYTE*)result.c_str()������ȷ
				result += 'A' + i - 1;
			}
		}

		lstCPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
		return 0;
	}

	int MakeDirectoryInfo(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//std::list<FILEINFPO> lstFileInfos;
// 		if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
// 			OutputDebugString(_T("��ǰ������ǻ�ȡ�ļ��б�����������󣡣�"));
// 			return -1;
// 		}
		//�ı䵱ǰ�Ĺ���Ŀ¼��ָ����·��
		if (_chdir(strPath.c_str()) != 0) {
			FILEINFPO finfo;
			finfo.HasNext = FALSE;

			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		
			OutputDebugString(_T("û��Ȩ�ޣ�����Ŀ¼����"));
			return -2;
		}
		//�����洢�ļ����ԵĽṹ�壬���ļ�����ʱ������ļ���С��
		_finddata_t fdata;
		//������������洢_findfirst�����ķ���ֵ����һ���������������ʶ��һ������״̬��
		int hfind{};
		//_findfirst������ʼ�ڵ�ǰĿ¼������ƥ��ָ��ͨ�����������"*"����ζ�������ļ������ļ���
		//����ҵ��ˣ�_findfirst�᷵��һ�����������һ���Ǹ��������������ҵ��ĵ�һ���ļ�����Ϣ��䵽fdata�ṹ����
		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("û���ҵ��κ��ļ�����"));
			FILEINFPO finfo;
			finfo.HasNext = FALSE;
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		int i{};
		do
		{
			i++;
			FILEINFPO finfo; //Ŀ¼�ṹ��
			//�ж��ǲ���Ŀ¼
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;

			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			//lstFileInfos.push_back(finfo);
			//�����������
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			//Dump((BYTE*)pack.Data(), pack.Size());
		} while (!_findnext(hfind, &fdata));
		//_findnext����ʹ��֮ǰ��_findfirst��ȡ���������hfind���������ҵ���һ���ļ���
		// ����ҵ��ˣ�_findnext����0������fdata�ṹ��Ϊ���ҵ����ļ�����Ϣ��
		FILEINFPO finfo;
		finfo.HasNext = FALSE;
		//�����������
		lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//CServerSocket::getInstance()->GetFilePath(strPath);
		//����ִ��һ�����򡢴�һ���ļ�����һ���ļ��У�������һ������Ӧ�ó���
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		//����Ӧ���
		lstCPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//CServerSocket::getInstance()->GetFilePath(strPath);
		long long data{};
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb"); //������ֻ����ʽ���ļ�

		//�ļ���ʧ�ܣ�����
		if (err != 0) {
			//Ӧ���
			lstCPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			return -1;
		}
		if (pFile != 0) {
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile); // ȡ���ļ����Ķ�ȡλ��,data�����ļ����ȡ�
			lstCPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET);

			char buffer[1024]{ "" };
			size_t rlen{};
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				//�����ļ�
				lstCPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			} while (rlen >= 1024);
			fclose(pFile);
		}
		//�ļ�����ʱ����һ����
		lstCPacket.push_back(CPacket(4, NULL, 0));
		return 0;
	}
	int MouseEvent(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
		//if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
		DWORD nFlags{};
		switch (mouse.nButton) {
		case 0://���
			nFlags = 1;
			break;
		case 1://�Ҽ�
			nFlags = 2;
			break;
		case 2://�м�
			nFlags = 4;
			break;
		case 3://û�а���
			nFlags = 8;
			break;
		}
			// ���ã��ƶ���ϵͳ����굽ָ������Ļ����
		if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//�������λ��
		switch (mouse.nAction)
		{
		case 0://����
			nFlags |= 0x10;
			break;
		case 1://˫��
			nFlags |= 0x20;
			break;
		case 2://����
			nFlags |= 0x40;
			break;
		case 3://�ſ�
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags)
		{
		case 0x21://���˫��
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //ִ���겻��break����һ�ν��е�����Ҳ����˫��
		case 0x11://�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://����ſ�
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://�Ҽ�˫��
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://�Ҽ��ſ�
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://�м�˫��
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://�м��ſ�
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://����������ƶ�
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
			//}
			lstCPacket.push_back(CPacket(5, NULL, 0));
		}
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//������һ��CImage��������һ�����ڴ���ͼ����࣬�ṩ��һϵ��ͼ������ķ���������ء�����ʹ���ͼ���
		CImage screen;//GDI,����
		//��ȡ������Ļ���豸������,�����ֵΪ NULL�� �� GetDC ������������Ļ�� DC��
		HDC hSreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hSreen, BITSPIXEL);//λ�� 24 ARGB 8888 32λ
		int nWidth = GetDeviceCaps(hSreen, HORZRES);//��
		int nHeight = GetDeviceCaps(hSreen, VERTRES);//��
		//������Ļ�Ŀ�ȡ��߶Ⱥ�λ��ȴ���һ��ͼ��
		screen.Create(nWidth, nHeight, nBitPerPixel);
		//ʹ��BitBlt��������Ļ���ݿ�����CImage�����С�ע�⣬����Ӳ�����˿�������Ĵ�СΪ1920x1020������ܲ�ƥ��������Ļ�ߴ硣
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hSreen, 0, 0, SRCCOPY);
		//BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hSreen, 0, 0, SRCCOPY);
		//�ͷ�֮ǰ��ȡ����ĻDC��û�д���
		ReleaseDC(NULL, hSreen);
		//����ȫ���ڴ�
		HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMen == NULL)return -1;

		IStream* pStream = NULL;
		//��������ȫ���ڴ����
		HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
		if (ret == S_OK) {
			//����ͼ������
			screen.Save(pStream, Gdiplus::ImageFormatJPEG);
			LARGE_INTEGER bg{};
			pStream->Seek(bg, STREAM_SEEK_SET, NULL); //���ļ�ָ�뻹ԭ���Ա�������
			//����ȫ���ڴ沢��ȡ����ָ��
			PBYTE pData = (PBYTE)GlobalLock(hMen);
			//��ȡ���ݴ�С
			SIZE_T nSize = GlobalSize(hMen);
			lstCPacket.push_back(CPacket(6, pData, nSize));
			//����ȫ���ڴ�
			GlobalUnlock(hMen);
		}
		screen.ReleaseDC(); //�ͷŵ�screen.GetDC()
		return 0;
	}

	int LockMachine(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//�����߳�
		//m_hWnd�����Ĵ��ھ��,NULL��INVALID_HANDLE_VALUE��ʾ������δ�����򴴽�ʧ�ܡ�
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
			//_beginthread(threadLockDlg, 0, NULL);//�����߳�
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
			TRACE("threadid = %d\r\n", threadid);
		}
		//Ӧ���
		lstCPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x2711);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0x2711); //������Ϣ
		lstCPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}
	int DeleteLockFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//���ͻ��˷��͵��ļ���ַ��Ϣ��ֵ��strPath
		//CServerSocket::getInstance()->GetFilePath(strPath);
		//����һ������
		//LPCWSTR spath = (LPCWSTR)strPath.c_str();

		//��������Ӣ�Ŀ��ԣ���������
		//TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());

		//��������
		TCHAR sPath[MAX_PATH] = _T("");
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
			sPath, sizeof(sPath) / sizeof(TCHAR));

		DeleteFile(sPath);
		lstCPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		lstCPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}
};