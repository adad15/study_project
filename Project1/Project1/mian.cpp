// 火车站卖票 A工人 B工人
#include <stdio.h>
#include <windows.h>
#include <process.h>
int iTickets = 100;
HANDLE g_hEvent;
DWORD WINAPI SellTicketA(void* lpParam)
{
	while (1)
	{
		WaitForSingleObject(g_hEvent, INFINITE);
		if (iTickets > 0)
		{
			Sleep(1);
			iTickets--;
			printf("A remain %d\n", iTickets);
		}
		else
		{
			break;
		}
		SetEvent(g_hEvent);
	}
	return 0;
}
DWORD WINAPI SellTicketB(void* lpParam)
{
	while (1)
	{
		WaitForSingleObject(g_hEvent, INFINITE);
		if (iTickets > 0)
		{
			Sleep(1);
			iTickets--;
			printf("B remain %d\n", iTickets);
		}
		else
		{
			break;
		}
		SetEvent(g_hEvent);
	}
	return 0;//0 内核对象被销毁
}
int main()
{
	HANDLE hThreadA, hThreadB;
	//创建线程
	hThreadA = CreateThread(NULL, 0, SellTicketA, NULL, 0, 0);// 2
	hThreadB = CreateThread(NULL, 0, SellTicketB, NULL, 0, 0);
	//在您的代码中，主线程（main 函数中的代码）并不需要与这些子线程进行直接的交互（如等待它们结束或获取它们的退出代码）。
	//因此，一旦线程被创建，它们的句柄对于主线程来说就不再有用。关闭这些句柄不会影响线程的执行。
	CloseHandle(hThreadA); //1
	CloseHandle(hThreadB);
	g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	SetEvent(g_hEvent);
	Sleep(4000);
	CloseHandle(g_hEvent);
	system("pause");
	return 0;
}