#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#define BUFFER_SIZE 2048000

#pragma pack(push)//�����ֽڶ����״̬
#pragma pack(1)

void Dump(BYTE* pData, size_t nSize);

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
	const char* Data(std::string& strOut) const{
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
	//std::string strOut;   //������������
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

//connect���������봦����
std::string GetErrorInfo(int wsaErrCode);

class CClientSockrt
{
public:
	static CClientSockrt* getInstance() {
		//��̬��Ա����û��thisָ�룬�޷�ֱ�ӷ���˽�ó�Ա����
		if (m_instance == NULL)
		{
			m_instance = new CClientSockrt();
		}

		return m_instance;
	}
	bool InitSocket() {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		//�׽��ֱ�����ʼ��
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		//ʹ�ô��ݵ�ip��ַ
		serv_addr.sin_addr.S_un.S_addr = htonl(m_nIP);
		serv_addr.sin_port = htons(m_nPort);
		if (serv_addr.sin_addr.S_un.S_addr == INADDR_NONE) {
			AfxMessageBox("ָ����ip��ַ�������ڣ�");
			return false;
		}

		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			//MessageBox��win32�ģ�����ʹ��AfxMessageBox��MFC�ģ�
			AfxMessageBox("����ʧ�ܣ�");
			//WSAGetLastError:���ش����
			TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}
	
	int DealCommond() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();//TODO�����̷߳�������ʱ���ܻ���ֳ�ͻ
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index{};
		while (true)
		{
			//ͨ������£���� send ���� recv �������� 0�����Ǿ���Ϊ�Զ˹ر������ӣ�
			// �������Ҳ�ر����Ӽ��ɣ�����ʵ�ʿ���ʱ����Ĵ����߼���
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0) && (int)index <= 0) {
				return -1;
			}
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len; //index ��ʾ���������е��ܳ���
			len = index;
			//�������صȺ��������������������
			//����ж��������һ�𣬵���������캯��len��ֵ��䣬��һ����ͷǰ�ķ�����+��һ�����ݰ�����
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("command %d\r\n", m_packet.sCmd);
			if (len > 0) {
				//��ȡ���һ�����ݰ������������ݰ�ǰ������ݣ��Ѻ����������ǰ�ᡣ
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
				TRACE("DealCommond m_packet.sCmd %d", m_packet.sCmd);
			}
		}
		return -1;
	}
	CPacket& GetPacket() {
		return m_packet;
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack) { //���ܳ������ã�Data�����ı�·pack�Ļ�����
		TRACE("m_sock = %d\r\n", m_sock);
		if (m_sock == -1) return false;
		std::string strout;
		pack.Data(strout);
		return send(m_sock, strout.c_str(), strout.size(), 0) > 0;
	}
	//������2���������е��������⴫��
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(int nIP, int nPort) {
		m_nIP = nIP;
		m_nPort = nPort;
	}

private:
	int m_nIP;//��ַ
	int m_nPort;//�˿�
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;  //��CPacket���б���Ҫ�и��ƹ��캯��
	CClientSockrt& operator=(const CClientSockrt& ss) {}
	CClientSockrt(const CClientSockrt& ss) {
		m_sock = ss.m_sock;
		m_nPort = ss.m_nPort;
		m_nIP = ss.m_nIP;
	}
	CClientSockrt() :
		m_nIP(INADDR_ANY), m_nPort(0){ 
		//��ʼ���׽��ֻ���
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}

		//��������ʼ��
		m_buffer.resize(BUFFER_SIZE);
	}
	~CClientSockrt() {
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
			CClientSockrt* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  //delete�������ͷ��ڴ�ռ䣬��������������������
		}
	}

	static CClientSockrt* m_instance;
	//����ʵ�ֵ����ĳ�ʼ��������
	class CHelper {
	public:
		CHelper() {
			CClientSockrt::getInstance();
		}
		~CHelper() {
			CClientSockrt::releaseInstance();
		}
	};
	static CHelper m_helper;
};