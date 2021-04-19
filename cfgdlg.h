#pragma once

#ifndef RTDISPLAY_HEADER_CFGDLG
#define RTDISPLAY_HEADER_CFGDLG

#include "common.h"

#define RTDISPLAY_RES_DLG_SETTINGS_CTL_PATHBOX      1101
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_SERIALCHECK  1102
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_BAUDRATEBOX  1103
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_DATABITSBOX  1104
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_PARITYBOX    1105
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_STOPBITSBOX  1106
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_FLOWCTLBOX   1107
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_OPENBTN      1108
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_ERRVALBOX    1109
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_IMGWIDTH     1110
#define RTDISPLAY_RES_DLG_SETTINGS_CTL_IMGHEIGHT    1111

// 0 = Cancelled, 1 = Succeeded, -1 = Failure
int RtdShowConfigWindow(RtdPortInfo_t* pinfPort);

#endif
