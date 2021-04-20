#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define TESTGEN_DELAY_PIXEL 0
#define TESTGEN_DELAY_LINE 10
#define TESTGEN_DELAY_FRAME 200

typedef struct TestBitmapInfo8bpp_struct {
	BITMAPINFOHEADER m_bmih;
	RGBQUAD m_arrColors[256];
} TestBitmapInfo8bpp_t;

static DWORD s_PrintError(DWORD nError, LPCSTR pcszContext);
static void s_PrintMessage(HANDLE hPrintTo, LPCSTR pcszFormat, UINT_PTR* arrInserts);
static LPCSTR s_GetErrorMessage(DWORD nErrorCode);

static const char s_cszPipeName[] = "\\\\.\\pipe\\TestImageGenerator";
static const char s_cDot = '.';
static const char s_cszFrameWritten[] = "!\r\n";

int main(int argc, char** argv) {
	HINSTANCE hInstance;
	HANDLE hHeap, hPipe, hStdOut;
	HBITMAP hBitmap;
	UINT_PTR arrInserts[4];
	BITMAP infBitmapOrig;
	TestBitmapInfo8bpp_t infBitmap8bpp;
	DWORD nBitmapStride, nColor, nBytesWritten;
	LONG nPosX, nPosY;
	HDC hScreenDC;
	BYTE* pBitmapData;

	hInstance = GetModuleHandleA(NULL);
	if (!hInstance) return s_PrintError(GetLastError(), "querying module handle");

	hHeap = GetProcessHeap();
	if (!hHeap) return s_PrintError(GetLastError(), "querying process heap");

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!hStdOut) return s_PrintError(GetLastError(), "opening standard output");

	hBitmap = LoadImageA(hInstance, MAKEINTRESOURCEA(1000), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (!hBitmap) return s_PrintError(GetLastError(), "loading test bitmap");

	if (!GetObjectA((HANDLE)hBitmap, sizeof(infBitmapOrig), &infBitmapOrig))
		return s_PrintError(GetLastError(), "querying test bitmap information");

	nBitmapStride = infBitmapOrig.bmWidth + (infBitmapOrig.bmWidth % 4);
	infBitmap8bpp.m_bmih.biBitCount = 8;
	infBitmap8bpp.m_bmih.biClrImportant = 0;
	infBitmap8bpp.m_bmih.biClrUsed = 256;
	infBitmap8bpp.m_bmih.biCompression = BI_RGB;
	infBitmap8bpp.m_bmih.biHeight = infBitmapOrig.bmHeight;
	infBitmap8bpp.m_bmih.biPlanes = 1;
	infBitmap8bpp.m_bmih.biSize = sizeof(infBitmap8bpp.m_bmih);
	infBitmap8bpp.m_bmih.biSizeImage = nBitmapStride * infBitmapOrig.bmHeight;
	infBitmap8bpp.m_bmih.biWidth = infBitmapOrig.bmWidth;
	infBitmap8bpp.m_bmih.biXPelsPerMeter = 0;
	infBitmap8bpp.m_bmih.biYPelsPerMeter = 0;

	for (nColor = 0; nColor < 256; nColor++) {
		infBitmap8bpp.m_arrColors[nColor].rgbRed = (BYTE)nColor;
		infBitmap8bpp.m_arrColors[nColor].rgbGreen = (BYTE)nColor;
		infBitmap8bpp.m_arrColors[nColor].rgbBlue = (BYTE)nColor;
		infBitmap8bpp.m_arrColors[nColor].rgbReserved = 0xFF;
	}

	arrInserts[0] = (UINT_PTR)infBitmap8bpp.m_bmih.biWidth;
	arrInserts[1] = (UINT_PTR)infBitmap8bpp.m_bmih.biHeight;
	arrInserts[2] = (UINT_PTR)nBitmapStride;
	arrInserts[3] = (UINT_PTR)infBitmap8bpp.m_bmih.biSizeImage;
	s_PrintMessage(hStdOut, "Image width: %1!u!, height: %2!u!, stride: %3!u!, size: %4!u!\r\n", arrInserts);

	pBitmapData = HeapAlloc(GetProcessHeap(), 0, infBitmap8bpp.m_bmih.biSizeImage);
	if (!pBitmapData) return s_PrintError(GetLastError(), "allocating memory for bitmap data");

	hScreenDC = GetDC(NULL);
	if (!hScreenDC) return s_PrintError(GetLastError(), "opening screen device context");

	if (!GetDIBits(hScreenDC, hBitmap, 0, infBitmapOrig.bmHeight, pBitmapData, (LPBITMAPINFO)&infBitmap8bpp, DIB_RGB_COLORS))
		return s_PrintError(GetLastError(), "copying bitmap data");

	ReleaseDC(NULL, hScreenDC);

	hPipe = CreateNamedPipeA(s_cszPipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE, 1, 0, 0, 0, NULL);
	if (!hPipe || (hPipe == INVALID_HANDLE_VALUE))
		return s_PrintError(GetLastError(), "creating named pipe");

	arrInserts[0] = (UINT_PTR)s_cszPipeName;
	s_PrintMessage(hStdOut, "Created pipe %1\r\n", arrInserts);

	if (!ConnectNamedPipe(hPipe, NULL))
		return s_PrintError(GetLastError(), "waiting for pipe connection");

	s_PrintMessage(hStdOut, "Pipe connected\r\n", arrInserts);

	for (;;) {
		for (nPosY = 0; nPosY < infBitmap8bpp.m_bmih.biHeight; nPosY++) {
#if TESTGEN_DELAY_PIXEL == 0
			for (nPosX = 0; nPosX < infBitmap8bpp.m_bmih.biWidth; ) {
				if (!WriteFile(hPipe, &pBitmapData[nPosX + nPosY * nBitmapStride], infBitmap8bpp.m_bmih.biWidth - nPosX, &nBytesWritten, NULL))
					return s_PrintError(GetLastError(), "writing pixels to pipe");
				nPosX += nBytesWritten;
			}
#else
			for (nPosX = 0; nPosX < infBitmap8bpp.m_bmih.biWidth; nPosX++) {
				if (!WriteFile(hPipe, &pBitmapData[nPosX + nPosY * nBitmapStride], 1, &nBytesWritten, NULL))
					return s_PrintError(GetLastError(), "writing pixel to pipe");
				Sleep(TESTGEN_DELAY_PIXEL);
			}
#endif

			if (!WriteFile(hStdOut, &s_cDot, 1, &nBytesWritten, NULL))
				return s_PrintError(GetLastError(), "reporting scanline written");

#if TESTGEN_DELAY_LINE != 0
			Sleep(TESTGEN_DELAY_LINE);
#endif
		}

		if (!WriteFile(hStdOut, s_cszFrameWritten, 3, &nBytesWritten, NULL))
			return s_PrintError(GetLastError(), "reporting frame written");

#if TESTGEN_DELAY_FRAME != 0
		Sleep(TESTGEN_DELAY_FRAME);
#endif
	}
}

static DWORD s_PrintError(DWORD nError, LPCSTR pcszContext) {
	UINT_PTR arrInserts[3];
	HANDLE hStdErr;

	hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	if (!hStdErr || (hStdErr == INVALID_HANDLE_VALUE)) {
		OutputDebugStringA("STD_ERROR_HANDLE invalid\r\n");
		ExitProcess(ERROR_INVALID_HANDLE);
	}

	arrInserts[0] = (UINT_PTR)nError;
	arrInserts[1] = (UINT_PTR)pcszContext;
	arrInserts[2] = (UINT_PTR)s_GetErrorMessage(nError);

	s_PrintMessage(hStdErr, "Error 0x%1!08X! while %2: %3", arrInserts);

	return nError;
}

static void s_PrintMessage(HANDLE hPrintTo, LPCSTR pcszFormat, UINT_PTR* arrInserts) {
	LPSTR pszMessage;
	DWORD nWritten, nError;

	if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		pcszFormat, 0, 0, (LPSTR)&pszMessage, 0, (va_list*)arrInserts))
	{
		if (!WriteFile(hPrintTo, pszMessage, lstrlenA(pszMessage), &nWritten, NULL)) {
			nError = GetLastError();
			OutputDebugStringA("Error writing to STD_ERROR_HANDLE\r\n");
			ExitProcess(nError);
		}
	} else {
		nError = GetLastError();
		OutputDebugStringA("Error formatting message ");
		OutputDebugStringA(pcszFormat);
		OutputDebugStringA("\r\n");
		ExitProcess(nError);
	}

	if (pszMessage) LocalFree((HLOCAL)pszMessage);
}

static LPCSTR s_GetErrorMessage(DWORD nErrorCode) {
	static char szBuffer[1024];

	if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, nErrorCode, 0, szBuffer,
		sizeof(szBuffer), NULL))
	{
		(void)lstrcpynA(szBuffer, "Unknown error", sizeof(szBuffer));
	}

	return szBuffer;
}
