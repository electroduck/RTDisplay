#include "cfgdlg.h"
#include "dispwnd.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nShowCmd) {
	DWORD nError;
	RtdPortInfo_t infPort;
	HWND hDispWnd;
	MSG msg;

	if (!RtdRegisterDisplayWindowClass())
		return s_ShowErrorMessage(GetLastError(), "registering display window class");

	switch (RtdShowConfigWindow(&infPort)) {
	case 0: return 0;
	case 1: break;
	case -1:
	default:
		return s_ShowErrorMessage(GetLastError(), "showing configuration window");
	}

	hDispWnd = RtdCreateDisplayWindow(&infPort);
	if (!hDispWnd)
		return s_ShowErrorMessage(GetLastError(), "creating display window");

	ShowWindow(hDispWnd, nShowCmd);

	while (GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return 0;
}

DWORD ShowErrorMessage(DWORD nError, LPCSTR pcszContext) {
	LPSTR pszDesc, pszMessage;
	void* arrArgs[3];

	arrArgs[0] = nError;
	arrArgs[1] = pcszContext;

	pszDesc = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, nError, NULL, (LPSTR)&pszDesc, NULL, NULL);
	arrArgs[2] = pszDesc;

	pszMessage = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY, (pszDesc == NULL) ?
		"Error 0x%1!08X! while %2" : "Error 0x%1!08X! while %2: %3", 0, 0, (LPSTR)&pszMessage, 0, (va_list*)arrArgs);
	
	MessageBoxA(NULL, pszMessage ? pszMessage : "Unknown error", "Error", MB_ICONERROR);

	if (pszDesc) LocalFree((HLOCAL)pszDesc);
	if (pszMessage) LocalFree((HLOCAL)pszMessage);

	return nError;
}
