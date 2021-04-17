cmake_minimum_required (VERSION 3.0)
project (RTDisplay)

add_executable (RTDisplay WIN32
	cfgdlg.c
	cfgdlg.h
	common.h
	dispwnd.c
	dispwnd.h
	main.c
	RTDisplay.rc
)

target_link_libraries (RTDisplay PRIVATE kernel32 user32 gdi32 comctl32)
