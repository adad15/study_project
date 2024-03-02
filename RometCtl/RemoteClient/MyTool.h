#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>

class CMyTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
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

	static int Bytes2Image(CImage& image, const std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str();
		//TODO: 存入CImage
		HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0); //分配全局内存
		if (hMen == NULL) {
			TRACE("内存不足了!\r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		//创建基于全局内存的流
		HRESULT hRet = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length{};
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg{};
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);//将文件指针还原
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);
		}
		return hRet;
	}
};