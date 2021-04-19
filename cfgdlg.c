#include "cfgdlg.h"

static INT_PTR s_ConfigDialogProc(HWND hDialogWindow, UINT nMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR s_OnCDCreate(HWND hWnd, RtdPortInfo_t* pinfPort);
static INT_PTR s_OnCDCommand(HWND hWnd, RtdPortInfo_t* pinfPort, WORD nControlID, WORD nNotifID, HWND hControlWindow);
static void s_ReportInvalidValue(HWND hDialogWindow, LPCSTR pcszControl, LPCSTR pcszValue, LPCSTR pcszProblem);
static LRESULT s_GetSelectedItem(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName);
static BOOL s_SetSelectedItem(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName, int nItem);
static BOOL s_PopulateComboBox(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName, LPCSTR* ppcszItems, SIZE_T nItems);

static LPCSTR s_arrParityItems[] = {
	"None",
	"Odd (XOR all bits)",
	"Even (XOR bits & invert)",
	"Mark (rarely used)",
	"Space (rarely used)"
};

static LPCSTR s_arrStopBitsItems[] = {
	"1 bit", "1.5 bits", "2 bits"
};

static LPCSTR s_arrFlowCtlItems[] = {
	"None",
	"XON/XOFF",
	"RTS/CTS",
	"DTR/DSR"
};

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

	case WM_CLOSE:
		if (!EndDialog(hDialogWindow, IDCLOSE))
			ShowErrorMessage(GetLastError(), "closing configuration dialog");
		return (INT_PTR)TRUE;

	default:
		return (INT_PTR)FALSE;
	}
}

static INT_PTR s_OnCDCreate(HWND hWnd, RtdPortInfo_t* pinfPort) {
	HICON hIconLarge, hIconSmall;
	HINSTANCE hInstance;
	LSTATUS nStatus;
	HKEY hCommKey;
	char szPortWin32Path[MAX_PATH];
	char szPortNTPath[MAX_PATH];
	DWORD nValueIndex, nPortNameLength, nPortNTPathLength, nValueType;
	LPSTR pszPortName;
	HWND hPathBoxWindow;

	nValueIndex = 0;
	hCommKey = NULL;
	hInstance = GetModuleHandleA(NULL);
	SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)pinfPort);

	// Large icon
	hIconLarge = LoadIconA(hInstance, MAKEINTRESOURCEA(RTDISPLAY_RES_ICON_SETTINGS));
	if (hIconLarge)
		SendMessageA(hWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIconLarge);
	else
		ShowErrorMessage(GetLastError(), "loading large settings icon");

	// Small icon
	hIconSmall = LoadImageA(hInstance, MAKEINTRESOURCEA(RTDISPLAY_RES_ICON_SETTINGS), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
	if (hIconSmall)
		SendMessageA(hWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIconSmall);
	else
		ShowErrorMessage(GetLastError(), "loading small settings icon");

	// COM port paths
	hPathBoxWindow = GetDlgItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PATHBOX);
	if (!hPathBoxWindow) {
		ShowErrorMessage(GetLastError(), "finding port path control");
		goto L_nopaths;
	}

	nStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hCommKey);
	if (nStatus != 0){
		ShowErrorMessage(nStatus, "opening COM port list");
		goto L_nopaths;
	}

	for (nValueIndex = 0; /* until fail */; nValueIndex++) {
		// Prepend \\.\ to the port name
		(void)lstrcpynA(szPortWin32Path, "\\\\.\\", sizeof(szPortWin32Path));
		pszPortName = szPortWin32Path + 4; // \\.\ takes up 4 chars
		nPortNameLength = sizeof(szPortWin32Path) - 4;
		nPortNTPathLength = sizeof(szPortNTPath);
		nValueType = REG_SZ;

		// Query the name
		nStatus = RegEnumValueA(hCommKey, nValueIndex, szPortNTPath, &nPortNTPathLength, NULL, &nValueType,
			pszPortName, &nPortNameLength);
		switch (nStatus) {
		case ERROR_SUCCESS:
			pszPortName[nPortNameLength] = '\0'; // function does not null-terminate automatically
			switch (SendMessageA(hPathBoxWindow, CB_ADDSTRING, 0, (LPARAM)szPortWin32Path)) {
			case CB_ERR:
				MessageBoxA(hWnd, "Error adding port name to path box", "Error", MB_ICONERROR);
				goto L_nopaths;

			case CB_ERRSPACE:
				MessageBoxA(hWnd, "Not enough space to add all port names to path box", "Error", MB_ICONERROR);
				goto L_nopaths;
			}
			break;

		case ERROR_NO_MORE_ITEMS: // that's it
			goto L_nopaths;

		default:
			ShowErrorMessage((DWORD)nStatus, "enumerating COM ports");
			goto L_nopaths;
		}
	}
L_nopaths:

	// Select the first port (if found)
	if (nValueIndex > 0) {
		if (SendMessageA(hPathBoxWindow, CB_SETCURSEL, 0, 0) == CB_ERR)
			MessageBoxA(hWnd, "Error selecting default port in path box", "Error", MB_ICONERROR);
	}

	// Is serial port
	if (!CheckDlgButton(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_SERIALCHECK, BST_CHECKED))
		ShowErrorMessage(GetLastError(), "checking serial checkbox");

	// Baud rate
	if (!SetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_BAUDRATEBOX, 9600, FALSE))
		ShowErrorMessage(GetLastError(), "setting default baud rate");

	// Data bits
	if (!SetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_DATABITSBOX, 8, FALSE))
		ShowErrorMessage(GetLastError(), "setting default data bit count");

	// Parity bit
	s_PopulateComboBox(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PARITYBOX, "parity type", s_arrParityItems, ARRAYSIZE(s_arrParityItems));
	s_SetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PARITYBOX, "parity type", (int)RTDISPLAY_PORT_PARITY_NONE);

	// Parity error value
	if (!SetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_ERRVALBOX, 0, FALSE))
		ShowErrorMessage(GetLastError(), "setting default parity error value");

	// Flow control
	s_PopulateComboBox(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_FLOWCTLBOX, "flow control type", s_arrFlowCtlItems,
		ARRAYSIZE(s_arrFlowCtlItems));
	s_SetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_FLOWCTLBOX, "flow control type", (int)RTDISPLAY_PORT_FLOWCTL_NONE);

	// Stop bits
	s_PopulateComboBox(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_STOPBITSBOX, "stop bit count", s_arrStopBitsItems,
		ARRAYSIZE(s_arrStopBitsItems));
	s_SetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_STOPBITSBOX, "stop bit count", (int)RTDISPLAY_PORT_STOPBITS_ONE);

	if (hCommKey) RegCloseKey(hCommKey);
	return (INT_PTR)TRUE;
}

static INT_PTR s_OnCDCommand(HWND hWnd, RtdPortInfo_t* pinfPort, WORD nControlID, WORD nNotifID, HWND hControlWindow) {
	UINT nLength, nValue;
	BOOL bTranslated;
	char szValue[256];
	LRESULT nSelItem;

	if (nControlID == RTDISPLAY_RES_DLG_SETTINGS_CTL_OPENBTN) {
		// Path
		nLength = GetDlgItemTextA(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PATHBOX, pinfPort->m_szPath, sizeof(pinfPort->m_szPath));
		if (nLength == 0) {
			s_ReportInvalidValue(hWnd, "port path", "", "Path cannot be empty");
			return (INT_PTR)TRUE;
		} else if (nLength >= MAX_PATH) {
			s_ReportInvalidValue(hWnd, "port path", pinfPort->m_szPath, "Path is too long (must be less than 260 characters)");
			return (INT_PTR)TRUE;
		}

		// Serial port checkbox
		switch (IsDlgButtonChecked(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_SERIALCHECK)) {
		case BST_CHECKED:
			pinfPort->m_bIsSerialPort = TRUE;
			break;

		case BST_UNCHECKED:
			pinfPort->m_bIsSerialPort = FALSE;
			break;

		case BST_INDETERMINATE:
			s_ReportInvalidValue(hWnd, "serial port checkbox", "indeterminate", "Must be yes (checked) or no (unchecked)");
			return (INT_PTR)TRUE;
		}

		if (pinfPort->m_bIsSerialPort) {
			// Baud rate
			pinfPort->m_nBaudRate = GetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_BAUDRATEBOX, &bTranslated, FALSE);
			if (!bTranslated) {
				GetDlgItemTextA(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_BAUDRATEBOX, szValue, sizeof(szValue));
				s_ReportInvalidValue(hWnd, "baud rate", szValue, "Must be a number");
				return (INT_PTR)TRUE;
			}

			// Data bits
			nValue = GetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_DATABITSBOX, &bTranslated, FALSE);
			if (!bTranslated || (nValue > 8)) {
				GetDlgItemTextA(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_DATABITSBOX, szValue, sizeof(szValue));
				s_ReportInvalidValue(hWnd, "data bits", szValue, bTranslated ? "Must be 8 or fewer" : "Must be a number");
				return (INT_PTR)TRUE;
			} else pinfPort->m_nDataBits = nValue;

			// Parity type
			nSelItem = s_GetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_PARITYBOX, "parity type");
			if (nSelItem == CB_ERR)
				return (INT_PTR)TRUE;
			else
				pinfPort->m_typeParity = (RtdPortParity_t)nSelItem;

			// Parity error value
			nValue = GetDlgItemInt(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_ERRVALBOX, &bTranslated, FALSE);
			if (!bTranslated || (nValue > 0xFF)) {
				GetDlgItemTextA(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_ERRVALBOX, szValue, sizeof(szValue));
				s_ReportInvalidValue(hWnd, "parity error value", szValue, bTranslated ? "Must be 255 or less" : "Must be a number");
				return (INT_PTR)TRUE;
			}
			else pinfPort->m_nParityErrorChar = nValue;

			// Flow control type
			nSelItem = s_GetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_FLOWCTLBOX, "flow control type");
			if (nSelItem == CB_ERR)
				return (INT_PTR)TRUE;
			else
				pinfPort->m_typeFlowCtl = (RtdPortFlowControl_t)nSelItem;

			// Stop bits type
			nSelItem = s_GetSelectedItem(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_STOPBITSBOX, "stop bit count");
			if (nSelItem == CB_ERR)
				return (INT_PTR)TRUE;
			else
				pinfPort->m_typeStop = (RtdPortStopBits_t)nSelItem;
		}

		// All OK
		if (!EndDialog(hWnd, RTDISPLAY_RES_DLG_SETTINGS_CTL_OPENBTN))
			ShowErrorMessage(GetLastError(), "ending configuration dialog");
		return (INT_PTR)TRUE;
	}

	return (INT_PTR)FALSE;
}

static void s_ReportInvalidValue(HWND hDialogWindow, LPCSTR pcszControl, LPCSTR pcszValue, LPCSTR pcszProblem) {
	LPSTR pszMessage;
	LPCVOID arrArgs[3];

	arrArgs[0] = (LPCVOID)pcszValue;
	arrArgs[1] = (LPCVOID)pcszControl;
	arrArgs[2] = (LPCVOID)pcszProblem;

	if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		"Invalid value \"%1\" for %2: %3", 0, 0, (LPSTR)&pszMessage, 16, (va_list*)arrArgs))
	{
		MessageBoxA(hDialogWindow, pszMessage, "Invalid value", MB_ICONERROR);
		LocalFree((HLOCAL)pszMessage);
	} else
		ShowErrorMessage(GetLastError(), "formatting invalid-value message");
}

static LRESULT s_GetSelectedItem(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName) {
	HWND hComboBoxWindow;
	LRESULT nSelectedItem;
	LPSTR pszContext;
	LPCVOID arrFormatArgs[1];
	DWORD nError;

	arrFormatArgs[0] = pcszControlName;

	hComboBoxWindow = GetDlgItem(hDialogWindow, nDialogItem);
	if (!hComboBoxWindow) {
		nError = GetLastError();
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"finding %1 control to query value", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
		{
			ShowErrorMessage(nError, pszContext);
			LocalFree((HLOCAL)pszContext);
		} else
			ShowErrorMessage(GetLastError(), "formatting control-not-found message");
		return CB_ERR;
	}

	nSelectedItem = SendMessageA(hComboBoxWindow, CB_GETCURSEL, 0, 0);
	if (nSelectedItem == CB_ERR) {
		nError = GetLastError();
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"querying value of %1 control", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
		{
			ShowErrorMessage(nError, pszContext);
			LocalFree((HLOCAL)pszContext);
		} else
			ShowErrorMessage(GetLastError(), "formatting value query error message");
		return CB_ERR;
	}

	return nSelectedItem;
}

static BOOL s_SetSelectedItem(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName, int nItem) {
	HWND hComboBoxWindow;
	LRESULT nResult;
	LPSTR pszContext;
	LPCVOID arrFormatArgs[2];
	DWORD nError;

	arrFormatArgs[0] = pcszControlName;
	arrFormatArgs[1] = (LPCVOID)(INT_PTR)nItem;

	hComboBoxWindow = GetDlgItem(hDialogWindow, nDialogItem);
	if (!hComboBoxWindow) {
		nError = GetLastError();
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"finding %1 control to set value", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
		{
			ShowErrorMessage(nError, pszContext);
			LocalFree((HLOCAL)pszContext);
		} else
			ShowErrorMessage(GetLastError(), "formatting control-not-found message");
		return FALSE;
	}

	nResult = SendMessageA(hComboBoxWindow, CB_SETCURSEL, (WPARAM)nItem, 0);
	if (nResult == CB_ERR) {
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"Error selecting item %2!u! of %1 control", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
		{
			MessageBoxA(hDialogWindow, pszContext, "Error", MB_ICONERROR);
			LocalFree((HLOCAL)pszContext);
		} else
			ShowErrorMessage(GetLastError(), "formatting selection setting error message");
		return FALSE;
	}

	return TRUE;
}

static BOOL s_PopulateComboBox(HWND hDialogWindow, int nDialogItem, LPCSTR pcszControlName, LPCSTR* ppcszItems, SIZE_T nItems) {
	HWND hComboBoxWindow;
	LRESULT nResult;
	LPSTR pszContext;
	LPCVOID arrFormatArgs[3];
	DWORD nError;

	arrFormatArgs[0] = pcszControlName;

	hComboBoxWindow = GetDlgItem(hDialogWindow, nDialogItem);
	if (!hComboBoxWindow) {
		nError = GetLastError();
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"finding %1 control to populate", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
		{
			ShowErrorMessage(nError, pszContext);
			LocalFree((HLOCAL)pszContext);
		} else
			ShowErrorMessage(GetLastError(), "formatting control-not-found message");
		return FALSE;
	}

	while (nItems > 0) {
		arrFormatArgs[1] = ppcszItems[0];
		arrFormatArgs[2] = "";

		nResult = SendMessageA(hComboBoxWindow, CB_ADDSTRING, 0, (LPARAM)ppcszItems[0]);
		switch (nResult) {
		case CB_ERRSPACE:
			arrFormatArgs[2] = ": Not enough space, drop-down is full";

		case CB_ERR:
			if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				"Error adding item \"%2\" to %1%3", 0, 0, (LPSTR)&pszContext, 0, (va_list*)arrFormatArgs))
			{
				MessageBoxA(hDialogWindow, pszContext, "Error adding item", MB_ICONERROR);
				LocalFree((HLOCAL)pszContext);
			} else
				ShowErrorMessage(GetLastError(), "formatting control-not-found message");
			return FALSE;
		}

		ppcszItems++;
		nItems--;
	}

	return TRUE;
}
