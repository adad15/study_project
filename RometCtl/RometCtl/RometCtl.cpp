// RometCtl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RometCtl.h"
#include "ServerSocket.h"
#include <direct.h>

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
//             CServerSocket* pserver = CServerSocket::getInstance();
//             int count{ 0 };
// 			if (pserver->InitSocket() == false) {
// 				MessageBox(NULL, _T("网络初始化异常，未能初始化，请检查网络！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
// 				exit(0);
// 			}
//             while (CServerSocket::getInstance() != NULL) {
//                 if (pserver->AcceptClient() == false) {
//                     if ((count++) > 3) {
// 						MessageBox(NULL, _T("多次无法正常接入用户，自动结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
//                         exit(0);
//                     }
// 					MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
//                     
//                 }
//                 int ret = pserver->DealCommond();
                //TODO:处理命令
 //           }
            int nCmd{ 1 };
            switch (nCmd)
            {
            case 1://查看磁盘分区
                MakeDriverInfo();
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
