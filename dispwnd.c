#include "dispwnd.h"

#ifdef _MSC_VER
#pragma warning(disable:6258)
#endif

typedef struct RtdBitmapInfo8bpp_struct {
	BITMAPINFOHEADER m_bmih;
	RGBQUAD m_arrColors[256];
} RtdBitmapInfo8bpp_t;

typedef struct RtdDispWndData_struct {
	HWND m_hDispWindow;
	HANDLE m_hPort, m_hThread;
	HBITMAP m_hBitmap;
	RtdPortInfo_t m_infPort;
	LPBYTE m_pPixelData;
	RtdBitmapInfo8bpp_t m_infBitmap;
	DWORD m_nBitmapStride, m_nThreadID;
	BOOL m_bThreadExitDesired;
} RtdDispWndData_t;

static LRESULT CALLBACK s_DisplayWindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
static LRESULT s_OnDWCreate(HWND hWnd, LPCREATESTRUCTA pCreateData);
static void s_OnDWDestroy(HWND hWnd, RtdDispWndData_t* pData);
static DWORD WINAPI s_ReaderThreadProc(LPVOID pDispWndData);
static void s_OnDWPaint(HWND hWnd, RtdDispWndData_t* pData);
static void s_OnDWDrawTo(HWND hWnd, RtdDispWndData_t* pData, HDC hDC);

static const char s_cszDispWindowClass[] = "RTDisplay_DisplayWindowClass";
static ATOM s_nDispWindowClass = 0;
static HINSTANCE s_hInstance = 0;
static HANDLE s_hHeap = 0;

BOOL RtdRegisterDisplayWindowClass(void) {
	WNDCLASSEXA wcxaDispWindow;

	if (s_hHeap == 0)
		s_hHeap = GetProcessHeap();

	if (s_hInstance == 0)
		s_hInstance = GetModuleHandleA(NULL);

	if (s_nDispWindowClass == 0) {
		ZeroMemory(&wcxaDispWindow, sizeof(wcxaDispWindow));
		wcxaDispWindow.cbSize = sizeof(wcxaDispWindow);
		wcxaDispWindow.cbWndExtra = sizeof(RtdDispWndData_t*);
		wcxaDispWindow.hbrBackground = GetStockObject(BLACK_BRUSH);
		wcxaDispWindow.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcxaDispWindow.hIcon = LoadIconA(s_hInstance, MAKEINTRESOURCEA(RTDISPLAY_RES_ICON_MONITOR));
		wcxaDispWindow.hIconSm = LoadImageA(s_hInstance, MAKEINTRESOURCEA(RTDISPLAY_RES_ICON_MONITOR), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		wcxaDispWindow.hInstance = s_hInstance;
		wcxaDispWindow.lpfnWndProc = s_DisplayWindowProc;
		wcxaDispWindow.lpszClassName = s_cszDispWindowClass;
		wcxaDispWindow.lpszMenuName = NULL;
		wcxaDispWindow.style = CS_HREDRAW | CS_VREDRAW;

		s_nDispWindowClass = RegisterClassExA(&wcxaDispWindow);
		if (s_nDispWindowClass == 0) return FALSE;
	}

	return TRUE;
}

HWND RtdCreateDisplayWindow(_In_ RtdPortInfo_t* pinfPort) {
	return CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, s_cszDispWindowClass, "RTDisplay", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		CW_USEDEFAULT, 800, 600, NULL, NULL, s_hInstance, (LPVOID)pinfPort);
}

static LRESULT CALLBACK s_DisplayWindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam) {
	RtdDispWndData_t* pData;
	pData = (RtdDispWndData_t*)GetWindowLongPtrA(hWnd, 0);

	switch (nMessage) {
	case WM_CREATE:
		return s_OnDWCreate(hWnd, (LPCREATESTRUCTA)lParam);

	case WM_DESTROY:
		s_OnDWDestroy(hWnd, pData);
		return 0;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_PAINT:
		s_OnDWPaint(hWnd, pData);
		return 0;

	case WM_PRINTCLIENT:
		s_OnDWDrawTo(hWnd, pData, (HDC)wParam);
		return 0;

	default:
		return DefWindowProcA(hWnd, nMessage, wParam, lParam);
	}
}

static LRESULT s_OnDWCreate(HWND hWnd, LPCREATESTRUCTA pCreateData) {
	RtdPortInfo_t* pinfPort;
	RtdDispWndData_t* pData;
	DCB dcb;
	DWORD nColor;

	pinfPort = (RtdPortInfo_t*)pCreateData->lpCreateParams;
	if (pinfPort == NULL) {
		MessageBoxA(NULL, "Null pointer passed to display window creation function", "Error", MB_ICONERROR);
		return -1;
	}

	pData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, sizeof(RtdDispWndData_t));
	if (!pData) {
		ShowErrorMessage(GetLastError(), "allocating memory for display window data");
		return -1;
	}

	pData->m_hDispWindow = hWnd;
	pData->m_infPort = *pinfPort;

	// Open port or pipe
	pData->m_hPort = CreateFileA(pinfPort->m_szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);
	if (pData->m_hPort == INVALID_HANDLE_VALUE) {
		ShowErrorMessage(GetLastError(), "opening serial port");
		goto L_error_free;
	}

	// Configure if serial port
	if (pinfPort->m_bIsSerialPort) {
		ZeroMemory(&dcb, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);

		if (!GetCommState(pData->m_hPort, &dcb)) {
			ShowErrorMessage(GetLastError(), "querying serial port state");
			goto L_error_close;
		}

		dcb.BaudRate = pinfPort->m_nBaudRate;
		dcb.ByteSize = pinfPort->m_nDataBits;
		dcb.ErrorChar = pinfPort->m_nParityErrorChar;
		dcb.fAbortOnError = 0;
		dcb.fBinary = 1;
		dcb.fDsrSensitivity = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_DTRDSR;
		dcb.fDtrControl = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_DTRDSR;
		dcb.fErrorChar = 1;
		dcb.fInX = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_XONXOFF;
		dcb.fNull = 0;
		dcb.fOutX = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_XONXOFF;
		dcb.fOutxCtsFlow = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_RTSCTS;
		dcb.fOutxDsrFlow = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_DTRDSR;
		dcb.fParity = pinfPort->m_typeParity != RTDISPLAY_PORT_PARITY_NONE;
		dcb.fRtsControl = pinfPort->m_typeFlowCtl == RTDISPLAY_PORT_FLOWCTL_RTSCTS;
		dcb.Parity = pinfPort->m_typeParity;
		dcb.StopBits = pinfPort->m_typeStop;

		if (!SetCommState(pData->m_hPort, &dcb)) {
			ShowErrorMessage(GetLastError(), "configuring serial port");
			goto L_error_close;
		}
	}

	// Bitmap info
	pData->m_nBitmapStride = pinfPort->m_nWidth + (pinfPort->m_nWidth % 4);
	pData->m_infBitmap.m_bmih.biBitCount = 8;
	pData->m_infBitmap.m_bmih.biClrImportant = 0;
	pData->m_infBitmap.m_bmih.biClrUsed = 256;
	pData->m_infBitmap.m_bmih.biCompression = BI_RGB;
	pData->m_infBitmap.m_bmih.biHeight = pinfPort->m_nHeight;
	pData->m_infBitmap.m_bmih.biPlanes = 1;
	pData->m_infBitmap.m_bmih.biSize = sizeof(pData->m_infBitmap.m_bmih);
	pData->m_infBitmap.m_bmih.biSizeImage = pData->m_nBitmapStride * pinfPort->m_nHeight;
	pData->m_infBitmap.m_bmih.biWidth = pinfPort->m_nWidth;
	pData->m_infBitmap.m_bmih.biXPelsPerMeter = 0;
	pData->m_infBitmap.m_bmih.biYPelsPerMeter = 0;

	// Palette
	for (nColor = 0; nColor < 256; nColor++) {
		pData->m_infBitmap.m_arrColors[nColor].rgbRed = (BYTE)nColor;
		pData->m_infBitmap.m_arrColors[nColor].rgbGreen = (BYTE)nColor;
		pData->m_infBitmap.m_arrColors[nColor].rgbBlue = (BYTE)nColor;
		pData->m_infBitmap.m_arrColors[nColor].rgbReserved = 0xFF;
	}

	// Create bitmap
	pData->m_hBitmap = CreateDIBSection(NULL, (BITMAPINFO*)&pData->m_infBitmap, DIB_RGB_COLORS, &pData->m_pPixelData, NULL, 0);
	if (!pData->m_hBitmap) {
		ShowErrorMessage(GetLastError(), "creating bitmap for received data");
		goto L_error_close;
	}

	memset(pData->m_pPixelData, 0xAA, pData->m_infBitmap.m_bmih.biSizeImage);
	//RtlSecureZeroMemory(pData->m_pPixelData, pData->m_infBitmap.m_bmih.biSizeImage);

	// Start reader thread
	pData->m_hThread = CreateThread(NULL, 0xFFFF, s_ReaderThreadProc, pData, 0, &pData->m_nThreadID);
	if (!pData->m_hThread) {
		ShowErrorMessage(GetLastError(), "starting data reception thread");
		goto L_error_delbitmap;
	}

	SetWindowLongPtrA(hWnd, 0, (LONG_PTR)pData);
	return 0;

L_error_delbitmap:
	DeleteObject(pData->m_hBitmap);
	pData->m_hBitmap = NULL;
L_error_close:
	CloseHandle(pData->m_hPort);
	pData->m_hPort = NULL;
L_error_free:
	HeapFree(s_hHeap, 0, pData);
	SetWindowLongPtrA(hWnd, 0, 0);
	return -1;
}

static void s_OnDWDestroy(HWND hWnd, RtdDispWndData_t* pData) {
	if (pData) {
		if (pData->m_hThread) {
			pData->m_bThreadExitDesired = TRUE;
			CancelSynchronousIo(pData->m_hThread);
			if (WaitForSingleObject(pData->m_hThread, 1000) == WAIT_TIMEOUT)
				TerminateThread(pData->m_hThread, ERROR_TIMEOUT);
			CloseHandle(pData->m_hThread);
			pData->m_hThread = NULL;
		}

		if (pData->m_hBitmap) {
			DeleteObject(pData->m_hBitmap);
			pData->m_hBitmap = NULL;
			pData->m_pPixelData = NULL;
		}

		if (pData->m_hPort && (pData->m_hPort != INVALID_HANDLE_VALUE)) {
			CloseHandle(pData->m_hPort);
			pData->m_hPort = NULL;
		}

		HeapFree(s_hHeap, 0, pData);
		pData = NULL;
	}

	SetWindowLongPtrA(hWnd, 0, 0);
}

static DWORD WINAPI s_ReaderThreadProc(LPVOID pDispWndData) {
	DWORD nBytesRead, nError;
	RtdDispWndData_t* pData;
	LONG nPosX, nPosY;

	pData = pDispWndData;
	if (!pData) {
		MessageBoxA(NULL, "Null pointer passed to ReaderThreadProc", "Error", MB_ICONERROR);
		return ERROR_INVALID_HANDLE;
	}

	// Continuously read frames
	for (;;) {
		// Loop through scanlines
		for (nPosY = 0; nPosY < pData->m_infBitmap.m_bmih.biHeight; nPosY++) {
			// Read each scanline before moving on to the next
			for (nPosX = 0, nBytesRead = 0; nPosX < pData->m_infBitmap.m_bmih.biWidth; nPosX += nBytesRead) {
				// Check before reading from port in case CancelSynchronousIo didn't have anything to cancel
				if (pData->m_bThreadExitDesired)
					return 0;

				// Read from port
				if (!ReadFile(pData->m_hPort, &pData->m_pPixelData[nPosY * pData->m_nBitmapStride + nPosX],
					pData->m_infBitmap.m_bmih.biWidth - nPosX, &nBytesRead, NULL))
				{
					nError = GetLastError();
					if ((nError == ERROR_OPERATION_ABORTED) && pData->m_bThreadExitDesired)
						return 0; // thread exit requested
					else {
						ShowErrorMessage(nError, "reading from port");
						return nError;
					}
				}

				// Update display
				if (!InvalidateRect(pData->m_hDispWindow, NULL, FALSE)) {
					nError = GetLastError();
					ShowErrorMessage(nError, "marking window for redraw");
					return nError;
				}
			}
		}
	}
}

static void s_OnDWPaint(HWND hWnd, RtdDispWndData_t* pData) {
	PAINTSTRUCT ps;

	if (!BeginPaint(hWnd, &ps)) {
		ShowErrorMessage(GetLastError(), "beginning window redraw");
		return;
	}

	s_OnDWDrawTo(hWnd, pData, ps.hdc);

	if (!EndPaint(hWnd, &ps)) {
		ShowErrorMessage(GetLastError(), "ending window redraw");
		return;
	}
}

static void s_OnDWDrawTo(HWND hWnd, RtdDispWndData_t* pData, HDC hDC) {
	RECT rcClient;
	HDC hTempDC;
	HGDIOBJ hOldBitmap;

	if (!GetClientRect(hWnd, &rcClient)) {
		ShowErrorMessage(GetLastError(), "querying window client area rectangle");
		return;
	}

	hTempDC = CreateCompatibleDC(hDC);
	if (!hTempDC) {
		ShowErrorMessage(GetLastError(), "creating temporary device context for bitmap");
		return;
	}

	hOldBitmap = SelectObject(hTempDC, pData->m_hBitmap);
	if (!hOldBitmap) {
		ShowErrorMessage(GetLastError(), "selecting bitmap into temporary device context");
		goto L_exit_deldc;
	}

	if (!BitBlt(hDC, rcClient.left + (rcClient.right - rcClient.left) / 2 - pData->m_infBitmap.m_bmih.biWidth / 2,
		rcClient.top + (rcClient.bottom - rcClient.top) / 2 - pData->m_infBitmap.m_bmih.biHeight / 2,
		pData->m_infBitmap.m_bmih.biWidth, pData->m_infBitmap.m_bmih.biHeight, hTempDC, 0, 0, SRCCOPY))
	{
		ShowErrorMessage(GetLastError(), "drawing bitmap to window");
		goto L_exit_desel;
	}
	
L_exit_desel:
	SelectObject(hTempDC, hOldBitmap);
	hOldBitmap = NULL;
L_exit_deldc:
	DeleteDC(hTempDC);
	hTempDC = NULL;
}
