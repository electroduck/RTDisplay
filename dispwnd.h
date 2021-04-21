#pragma once

#ifndef RTDISPLAY_HEADER_DISPWND
#define RTDISPLAY_HEADER_DISPWND

#include "common.h"

#define RTDISPLAY_RES_MENU_DISPWND_ITEM_SAVE_IMAGE   1201
#define RTDISPLAY_RES_MENU_DISPWND_ITEM_SAVE_BINARY	 1202
#define RTDISPLAY_RES_MENU_DISPWND_ITEM_SAVE_TEXT    1203

typedef void (* RtdDisplayWindowDestroyCB_t)(HWND hDisplayWindow);

BOOL RtdRegisterDisplayWindowClass(void);
HWND RtdCreateDisplayWindow(_In_ RtdPortInfo_t* pinfPort);
void RtdSetDisplayWindowDestroyCB(HWND hWnd, RtdDisplayWindowDestroyCB_t procDestroyCB);

#endif
