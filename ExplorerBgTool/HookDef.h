/*
* Hook函数声明
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#pragma once
#include "framework.h"

#define CreateMHook(fun, mfun, ofun, index) \
if (MH_CreateHook(&fun, &mfun, (LPVOID*)&ofun) != MH_OK) \
MessageBoxW(0, (L"Failed to create hook <" + std::to_wstring(index)).c_str(), L"MTweaker Error", MB_ICONERROR | MB_OK) \

//Hook的原始函数
#pragma region Original Function

typedef HWND(WINAPI* O_CreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD,
    int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
O_CreateWindowExW _CreateWindowExW_;

typedef BOOL(WINAPI* O_DestroyWindow)(HWND);
O_DestroyWindow _DestroyWindow_;

typedef HDC(WINAPI* O_BeginPaint)(HWND, LPPAINTSTRUCT);
O_BeginPaint _BeginPaint_;

typedef int(WINAPI* O_FillRect)(HDC, const RECT*, HBRUSH);
O_FillRect _FillRect_;

typedef HDC(WINAPI* O_CreateCompatibleDC)(HDC);
O_CreateCompatibleDC _CreateCompatibleDC_;

#pragma endregion

extern HWND MyCreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD,
    int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

extern BOOL MyDestroyWindow(HWND);

extern HDC MyBeginPaint(HWND, LPPAINTSTRUCT);

extern int MyFillRect(HDC, const RECT*, HBRUSH);

extern HDC MyCreateCompatibleDC(HDC);