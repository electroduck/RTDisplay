#include "cfgdlg.h"
#include "dispwnd.h"

// Returns the same error code passed in
static DWORD s_ShowErrorMessage(DWORD nError, LPCSTR pcszContext);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nShowCmd) {
	DWORD nError;
	RtdPortInfo_t infPort;
	HWND hDispWnd;
	MSG msg;

	if (!RtdRegisterDisplayWindowClass())
		return s_ShowErrorMessage(GetLastError(), "registering display window class");

	if (!RtdShowConfigWindow(&infPort))
		return s_ShowErrorMessage(GetLastError(), "showing configuration window");

	hDispWnd = RtdCreateDisplayWindow(&infPort);
	if (!hDispWnd)
		return s_ShowErrorMessage(GetLastError(), "creating display window");
}
