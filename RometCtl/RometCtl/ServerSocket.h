#pragma once
#include "pch.h"
#include "framework.h"
#define BUFFER_SIZE 4096

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


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		//��̬��Ա����û��thisָ�룬�޷�ֱ�ӷ���˽�ó�Ա����
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}

		return m_instance;
	}
	bool InitSocket() {
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		
		if (listen(m_sock, 1) == -1) return false;//ֻ����һ�����ƶ˽��п��ƣ�������Ϊ1

		return true;
	}
	bool AcceptClient() {
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		if (m_client == -1) return false;
		//closesocket(m_client);
		return true;
	}
	int DealCommond(){
		if (m_client == -1) return false;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index{};  
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index;
			//�������صȺ��������������������
			//����ж��������һ�𣬵���������캯��len��ֵ��䣬��һ����ͷǰ�ķ�����+��һ�����ݰ�����
			m_packet = CPacket((BYTE*)buffer, len); 
			if (len > 0) {
				//��ȡ���һ�����ݰ������������ݰ�ǰ������ݣ��Ѻ����������ǰ�ᡣ
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
	}

	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) { //���ܳ������ã�Data�����ı�·pack�Ļ�����
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	//������2���������е��������⴫��
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

private:
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;  //��CPacket���б���Ҫ�и��ƹ��캯��
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		//�׽��ֱ�����ʼ��
		m_client = INVALID_SOCKET;  //��������-1.

		//��ʼ���׽��ֻ���
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		//�׽��ֱ�����ʼ��
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup(); //��WSAStartupһһ��Ӧ
	}
	BOOL InitSockEnv() {
		// �����׽��ֿ�
		WORD wVersionRequested;//����ָ����������� Winsock �汾
		WSADATA wsaData;    //���� Windows Sockets ʵ�ֵ���ϸ��Ϣ
		int err;
		//ͨ��MAKEWORD(1, 1)ָ���������� Winsock 1.1 �汾
		wVersionRequested = MAKEWORD(1, 1);
		// ��ʼ���׽��ֿ�
		//�������Ҫ��������ʹ�õ� Winsock �汾��һ�� `WSADATA` �ṹ��ָ�롣�������� `0` ��ʾ�ɹ���
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			return FALSE;
		}
		//���д����� Winsock ���ʵ�ʰ汾�Ƿ�������İ汾���ݡ�
		if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
		{
			//����汾�����ݣ������WSACleanup����ж��Winsock�⡣
			WSACleanup();
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  //delete�������ͷ��ڴ�ռ䣬��������������������
		}
	}

	static CServerSocket* m_instance;
	//����ʵ�ֵ����ĳ�ʼ��������
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};