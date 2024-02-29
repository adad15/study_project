#include "pch.h"
#include "Command.h"

CCommand::CCommand():threadid(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1,&CCommand::MakeDriverInfo},
		{2,&CCommand::MakeDirectoryInfo},
		{3,&CCommand::RunFile},
		{4,&CCommand::DownloadFile},
		{5,&CCommand::MouseEvent},
		{6,&CCommand::SendScreen},
		{7,&CCommand::LockMachine},
		{8,&CCommand::UnlockMachine},
		{9,&CCommand::DeleteLockFile},
		{1981,&CCommand::TestConnect},
		{-1,NULL}
	};
	//使用映射表提高查找的效率
	for (int i{}; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstCPacket,CPacket& inPacket)
{
	//哈希的方式，数据量增大，查找的效率不会跟swich那样骤增。
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	//it->second表示函数指针
	//this->*: 这个部分可能是最难理解的。this是一个指向当前对象的指针。
	//在C++中，成员函数指针需要使用->*操作符来间接访问。
	//如果有一个指向成员函数的指针，你需要通过一个对象（在这里是this指针指向的对象）来调用它。
	return (this->*it->second)(lstCPacket, inPacket);
}