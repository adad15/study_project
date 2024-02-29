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
	//ʹ��ӳ�����߲��ҵ�Ч��
	for (int i{}; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstCPacket,CPacket& inPacket)
{
	//��ϣ�ķ�ʽ�����������󣬲��ҵ�Ч�ʲ����swich����������
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	//it->second��ʾ����ָ��
	//this->*: ������ֿ������������ġ�this��һ��ָ��ǰ�����ָ�롣
	//��C++�У���Ա����ָ����Ҫʹ��->*����������ӷ��ʡ�
	//�����һ��ָ���Ա������ָ�룬����Ҫͨ��һ��������������thisָ��ָ��Ķ�������������
	return (this->*it->second)(lstCPacket, inPacket);
}