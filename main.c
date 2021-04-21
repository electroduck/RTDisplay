#include "cfgdlg.h"
#include "dispwnd.h"

static void s_DisplayWindowDestroyed(HWND hDisplayWindow);

void main(void) {
	RtdPortInfo_t infPort;
	HWND hDispWnd;
	MSG msg;

	if (!RtdRegisterDisplayWindowClass())
		ExitProcess(ShowErrorMessage(GetLastError(), "registering display window class"));

	switch (RtdShowConfigWindow(&infPort)) {
	case 0: ExitProcess(0);
	case 1: break;
	case -1:
	default:
		ExitProcess(ShowErrorMessage(GetLastError(), "showing configuration window"));
	}

	hDispWnd = RtdCreateDisplayWindow(&infPort);
	if (!hDispWnd)
		ExitProcess(ShowErrorMessage(GetLastError(), "creating display window"));

	RtdSetDisplayWindowDestroyCB(hDispWnd, s_DisplayWindowDestroyed);
	ShowWindow(hDispWnd, SW_SHOW);

	while (GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	ExitProcess(0);
}

#ifdef RTDISPLAY_ENABLE_WINMAIN
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pszCmdLine, int nCmdShow) {
	main();
	return 0;
}
#endif

DWORD ShowErrorMessage(DWORD nError, LPCSTR pcszContext) {
	LPSTR pszDesc, pszMessage;
	void* arrArgs[3];

	arrArgs[0] = (void*)(UINT_PTR)nError;
	arrArgs[1] = (void*)pcszContext;

	if ((nError == 0) || !FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS, NULL, nError, 0, (LPSTR)&pszDesc, 0, NULL)) pszDesc = NULL;

	arrArgs[2] = pszDesc;
	if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		(pszDesc == NULL) ? "Error 0x%1!08X! while %2" : "Error 0x%1!08X! while %2: %3", 0, 0, (LPSTR)&pszMessage, 0,
		(va_list*)arrArgs)) pszMessage = NULL;
	
	MessageBoxA(NULL, pszMessage ? pszMessage : "Unknown error", "Error", MB_ICONERROR);

	if (pszDesc) LocalFree((HLOCAL)pszDesc);
	if (pszMessage) LocalFree((HLOCAL)pszMessage);

	return nError;
}

static void s_DisplayWindowDestroyed(HWND hDisplayWindow) {
	PostQuitMessage(0);
}
