#pragma once

#ifndef RTDISPLAY_HEADER_COMMON
#define RTDISPLAY_HEADER_COMMON

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define RTDISPLAY_RES_ICON_MONITOR   1000
#define RTDISPLAY_RES_ICON_SETTINGS  1001
#define RTDISPLAY_RES_DLG_SETTINGS   1100
#define RTDISPLAY_RES_MENU_DISPWND   1200

typedef enum RtdPortParity_enum {
	RTDISPLAY_PORT_PARITY_NONE = NOPARITY,
	RTDISPLAY_PORT_PARITY_ODD = ODDPARITY,
	RTDISPLAY_PORT_PARITY_EVEN = EVENPARITY,
	RTDISPLAY_PORT_PARITY_MARK = MARKPARITY,
	RTDISPLAY_PORT_PARITY_SPACE = SPACEPARITY
} RtdPortParity_t;

typedef enum RtdPortStopBits_enum {
	RTDISPLAY_PORT_STOPBITS_ONE = ONESTOPBIT,
	RTDISPLAY_PORT_STOPBITS_ONEPOINTFIVE = ONE5STOPBITS,
	RTDISPLAY_PORT_STOPBITS_TWO = TWOSTOPBITS
} RtdPortStopBits_t;

typedef enum RtdPortFlowControl_enum {
	RTDISPLAY_PORT_FLOWCTL_NONE,
	RTDISPLAY_PORT_FLOWCTL_XONXOFF,
	RTDISPLAY_PORT_FLOWCTL_RTSCTS,
	RTDISPLAY_PORT_FLOWCTL_DTRDSR
} RtdPortFlowControl_t;

typedef struct RtdPortInfo_struct {
	char m_szPath[MAX_PATH];
	BOOL m_bIsSerialPort;

	// Port configuration
	DWORD m_nBaudRate;
	BYTE m_nDataBits, m_nParityErrorChar;
	RtdPortParity_t m_typeParity;
	RtdPortStopBits_t m_typeStop;
	RtdPortFlowControl_t m_typeFlowCtl;

	// Image configuration
	DWORD m_nWidth, m_nHeight;
} RtdPortInfo_t;

// Returns the same error code passed in
DWORD ShowErrorMessage(DWORD nError, LPCSTR pcszContext);

#endif
