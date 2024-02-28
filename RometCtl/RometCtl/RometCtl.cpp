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
                    ret = cmd.ExcuteCommand(ret);
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