#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void s_PrintMessage(LPCSTR pcszFormat, void** arrInserts);
static LPCSTR s_GetErrorMessage(DWORD nErrorCode);

int main(int argc, char** argv) {
	HINSTANCE hInstance;
	HBITMAP hBitmap;
	UINT_PTR arrInserts[4];

	hInstance = GetModuleHandleA(NULL);
	if (!hInstance) {
		arrInserts[0] = (UINT_PTR)GetLastError();
		arrInserts[1] = s_GetErrorMessage((DWORD)arrInserts[0]);
		s_PrintMessage("Error 0x%1!08X! querying module handle: %2", arrInserts);
		return (int)arrInserts[0];
	}

	hBitmap = LoadImageA(hInstance, MAKEINTRESOURCEA(1000), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (!hBitmap) {
		arrInserts[0] = (UINT_PTR)GetLastError();
		arrInserts[1] = s_GetErrorMessage((DWORD)arrInserts[0]);
		s_PrintMessage("Error 0x%1!08X! loading test bitmap: %2", arrInserts);
		return (int)arrInserts[0];
	}


}

static void s_PrintMessage(LPCSTR pcszFormat, void** arrInserts) {
	LPSTR pszMessage;
	HANDLE hStdErr;
	DWORD nWritten, nError;

	hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	if (!hStdErr || (hStdErr == INVALID_HANDLE_VALUE)) {
		OutputDebugStringA("STD_ERROR_HANDLE invalid\r\n");
		ExitProcess(INVALID_HANDLE_VALUE);
	}

	if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		pcszFormat, 0, 0, (LPSTR)&pszMessage, 0, (va_list*)arrInserts))
	{
		if (!WriteFile(hStdErr, pszMessage, lstrlenA(pszMessage), &nWritten, NULL)) {
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
