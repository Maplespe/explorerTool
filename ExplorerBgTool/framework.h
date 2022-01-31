#pragma once

#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

// 全局模块句柄 Global module handle
extern HMODULE g_hModule;