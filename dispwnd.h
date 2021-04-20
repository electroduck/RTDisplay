#pragma once

#ifndef RTDISPLAY_HEADER_DISPWND
#define RTDISPLAY_HEADER_DISPWND

#include "common.h"

typedef void (* RtdDisplayWindowDestroyCB_t)(HWND hDisplayWindow);

BOOL RtdRegisterDisplayWindowClass(void);
HWND RtdCreateDisplayWindow(_In_ RtdPortInfo_t* pinfPort);
void RtdSetDisplayWindowDestroyCB(HWND hWnd, RtdDisplayWindowDestroyCB_t procDestroyCB);

#endif
