#include "dispwnd.h"

typedef struct RtdDispWndData_struct {
	HANDLE m_hPort;
} RtdDispWndData_t;

static LRESULT CALLBACK s_DisplayWindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
static LRESULT s_OnDWCreate(HWND hWnd, LPCREATESTRUCTA pCreateData);
static void s_OnDWDestroy(HWND hWnd, RtdDispWndData_t* pData);

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
	pData = GetWindowLongPtrA(hWnd, 0);

	switch (nMessage) {
	case WM_CREATE:
		return s_OnDWCreate(hWnd, (LPCREATESTRUCTA)lParam);

	case WM_DESTROY:
		s_OnDWDestroy(hWnd, pData);
		return 0;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	default:
		return DefWindowProcA(hWnd, nMessage, wParam, lParam);
	}
}

static LRESULT s_OnDWCreate(HWND hWnd, LPCREATESTRUCTA pCreateData) {
	RtdPortInfo_t* pinfPort;
	RtdDispWndData_t* pData;
	DCB dcb;

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

	pData->m_hPort = CreateFileA(pinfPort->m_szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);
	if (pData->m_hPort == INVALID_HANDLE_VALUE) {
		ShowErrorMessage(GetLastError(), "opening serial port");
		goto L_error_free;
	}

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

	SetWindowLongPtrA(hWnd, 0, (LONG_PTR)pData);
	return 0;

L_error_close:
	CloseHandle(pData->m_hPort);
	pData->m_hPort = NULL;
L_error_free:
	HeapFree(s_hHeap, 0, pData);
	return -1;
}

static void s_OnDWDestroy(HWND hWnd, RtdDispWndData_t* pData) {
	if (pData->m_hPort && (pData->m_hPort != INVALID_HANDLE_VALUE)) {
		CloseHandle(pData->m_hPort);
		pData->m_hPort = NULL;
	}

	HeapFree(s_hHeap, 0, pData);
	SetWindowLongPtrA(hWnd, 0, 0);
}
