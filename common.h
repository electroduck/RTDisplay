#pragma once

#ifndef RTDISPLAY_HEADER_COMMON
#define RTDISPLAY_HEADER_COMMON

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define RTDISPLAY_RES_ICON_MONITOR  1000
#define RTDISPLAY_RES_ICON_SETTINGS 1001

typedef struct RtdPortInfo_struct {
	char m_szPath[MAX_PATH];
	BOOL m_bIsSerialPort;
	DCB m_dcb;
} RtdPortInfo_t;

// Returns the same error code passed in
DWORD ShowErrorMessage(DWORD nError, LPCSTR pcszContext);

#endif
