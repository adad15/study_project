#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)//保存字节对齐的状态
#pragma pack(1)
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	//打包数据
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFE;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		size_t a = strData.size();
		for (size_t j{}; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	//解析数据
	CPacket(const BYTE* pData, size_t& nSize) {
		//找包头
		size_t i{};
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFE) {
				sHead = *(WORD*)(pData + i);
				i += 2;  //i向后移动两字节，把包头部分除去
				break;
			}
			if (i + 4 + 2 + 2 > nSize) { //包数据可能不全，或者包头未能全部接收到
				nSize = 0;
				return;
			}
		}
		nLength = *(DWORD*)(pData + i); i += 4; //i向后移动4字节，除去长度数据
		if (nLength + i > nSize) { //包未完全接收到，就返回解析失败
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2; //i向后移动2字节，除去命令长度
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); //调整字符串的长度
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //复制字符串
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum{};
		for (size_t j{}; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //不要忘记包头前面可能残余数据,一起销毁掉。
			//nSize = nLength + 2 + 4; //包头部分+长度部分
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;   //实现连等
	}
	int Size() {//包数据的大小
		return nLength + 2 + 4;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str();
	}
public:
	WORD sHead;           //包头 0xFEFE
	DWORD nLength;        //包长度（从控制命令开始，到和校验结束）
	WORD sCmd;            //控制命令
	std::string strData;  //包数据
	WORD sSum;            //和校验，只检验包数据的长度
	std::string strOut;   //整个包的数据
};
#pragma pack(pop)//还原字节对齐的状态


typedef struct MouseEvent
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //点击、移动、双击
	WORD nButton; //左键、中键
	POINT ptXY;
}MOUSEEV, * PMOUSEEV;

//定义文件结构体
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录
	BOOL HasNext;//是否还有后续
	char szFileName[256];//文件名
}FILEINFPO, * PFILEINFPO;