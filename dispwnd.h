#pragma once

#ifndef RTDISPLAY_HEADER_DISPWND
#define RTDISPLAY_HEADER_DISPWND

#include "common.h"

BOOL RtdRegisterDisplayWindowClass(void);
HWND RtdCreateDisplayWindow(_In_ RtdPortInfo_t* pinfPort);

#endif
