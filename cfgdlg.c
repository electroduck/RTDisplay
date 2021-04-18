#include "cfgdlg.h"

static INT_PTR s_ConfigDialogProc(HWND hDialogWindow, UINT nMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR s_OnCDCreate(HWND hWnd, RtdPortInfo_t* pinfPort);
static INT_PTR s_OnCDCommand(HWND hWnd, RtdPortInfo_t* pinfPort, WORD nControlID, WORD nNotifID, HWND hControlWindow);

int RtdShowConfigWindow(RtdPortInfo_t* pinfPort) {
	INT_PTR nResult;

	nResult = DialogBoxParamA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(RTDISPLAY_RES_DLG_SETTINGS), NULL,
		s_ConfigDialogProc, (LPARAM)pinfPort);
	
	switch (nResult) {
	case IDCANCEL:
	case IDCLOSE:
		return 0;

	case RTDISPLAY_RES_DLG_SETTINGS_CTL_OPENBTN:
		return 1;

	case -1:
	default:
		return -1;
	}
}

static INT_PTR s_ConfigDialogProc(HWND hDialogWindow, UINT nMessage, WPARAM wParam, LPARAM lParam) {
	RtdPortInfo_t* pinfPort;
	pinfPort = (RtdPortInfo_t*)GetWindowLongPtrA(hDialogWindow, GWLP_USERDATA);

	switch (nMessage) {
	case WM_INITDIALOG:
		return s_OnCDCreate(hDialogWindow, (RtdPortInfo_t*)lParam);

	case WM_COMMAND:
		return s_OnCDCommand(hDialogWindow, pinfPort, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

	default:
		return (INT_PTR)FALSE;
	}
}

static INT_PTR s_OnCDCreate(HWND hWnd, RtdPortInfo_t* pinfPort) {
	SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)pinfPort);
	return TRUE;
}

static INT_PTR s_OnCDCommand(HWND hWnd, RtdPortInfo_t* pinfPort, WORD nControlID, WORD nNotifID, HWND hControlWindow) {
	if (nControlID == RTDISPLAY_RES_DLG_SETTINGS_CTL_OPENBTN) {
		GetDlgItemTextA(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PATHBOX, pinfPort->m_szPath, sizeof(pinfPort->m_szPath));
	}
}
