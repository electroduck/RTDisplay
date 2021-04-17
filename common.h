#pragma once

#ifndef RTDISPLAY_HEADER_COMMON
#define RTDISPLAY_HEADER_COMMON

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct RtdPortInfo_struct {
	char m_szPath[MAX_PATH];
	BOOL m_bIsSerialPort;
	DCB m_dcb;
} RtdPortInfo_t;

#endif
