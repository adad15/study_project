#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)//�����ֽڶ����״̬
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
	//�������
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

	//��������
	CPacket(const BYTE* pData, size_t& nSize) {
		//�Ұ�ͷ
		size_t i{};
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFE) {
				sHead = *(WORD*)(pData + i);
				i += 2;  //i����ƶ����ֽڣ��Ѱ�ͷ���ֳ�ȥ
				break;
			}
			if (i + 4 + 2 + 2 > nSize) { //�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
				nSize = 0;
				return;
			}
		}
		nLength = *(DWORD*)(pData + i); i += 4; //i����ƶ�4�ֽڣ���ȥ��������
		if (nLength + i > nSize) { //��δ��ȫ���յ����ͷ��ؽ���ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2; //i����ƶ�2�ֽڣ���ȥ�����
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); //�����ַ����ĳ���
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //�����ַ���
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum{};
		for (size_t j{}; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //��Ҫ���ǰ�ͷǰ����ܲ�������,һ�����ٵ���
			//nSize = nLength + 2 + 4; //��ͷ����+���Ȳ���
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
		return *this;   //ʵ������
	}
	int Size() {//�����ݵĴ�С
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
	WORD sHead;           //��ͷ 0xFEFE
	DWORD nLength;        //�����ȣ��ӿ������ʼ������У�������
	WORD sCmd;            //��������
	std::string strData;  //������
	WORD sSum;            //��У�飬ֻ��������ݵĳ���
	std::string strOut;   //������������
};
#pragma pack(pop)//��ԭ�ֽڶ����״̬


typedef struct MouseEvent
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //������ƶ���˫��
	WORD nButton; //������м�
	POINT ptXY;
}MOUSEEV, * PMOUSEEV;

//�����ļ��ṹ��
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	BOOL IsDirectory;//�Ƿ�ΪĿ¼
	BOOL HasNext;//�Ƿ��к���
	char szFileName[256];//�ļ���
}FILEINFPO, * PFILEINFPO;