/*
* 文件资源管理器背景工具扩展
* 
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/
#include "pch.h"
#include <string>
#include <vector>

//GDI 相关 Using GDI
#include <comdef.h>
#include <gdiplus.h>
#pragma comment(lib, "GdiPlus.lib")
//
//minihook
#include "MinHook.h"

#include "WinAPI.h"
#include "HookDef.h"

//AlphaBlend
#pragma comment(lib, "Msimg32.lib")  

using namespace Gdiplus;

extern HWND MyCreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD,
    int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

extern BOOL MyDestroyWindow(HWND);

extern HDC MyBeginPaint(HWND, LPPAINTSTRUCT);

extern int MyFillRect(HDC, const RECT*, HBRUSH);

extern HDC MyCreateCompatibleDC(HDC);

struct MyData
{
    HWND hWnd = 0;
    HDC hDC = 0;
    SIZE size = { 0,0 };
    int ImgIndex = 0;
};

//全局变量
#pragma region GlobalVariable

HMODULE g_hModule = NULL; // 全局模块句柄 Global module handle

ULONG_PTR m_gdiplusToken; // GDI初始化标志 GDI Init flag

std::vector<BitmapGDI*> pBgBmp; // 背景图

std::vector<MyData> DUIList;//dui句柄列表 dui handle list

/* 0 = Left top
*  1 = Right top
*  2 = Left bottom
*  3 = Right bottom
*/
int ImgPosMode = 0;//图片定位方式 Image position mode type

bool Random = true;//随机显示图片 Random pictures

#pragma endregion

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    g_hModule = hModule;
    DisableThreadLibraryCalls(hModule);
    return TRUE;
}

//Hook入口点 EasyHook EntryPoint
extern "C" __declspec(dllexport) bool __stdcall InjectionEntryPoint()
{
    //初始化 Gdiplus Init GdiPlus
    GdiplusStartupInput StartupInput;
    int ret = GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);

    //加载配置 Load config
    std::wstring cfgPath = GetCurDllDir() + L"\\config.ini";
    Random = GetIniString(cfgPath, L"image", L"random") == L"true" ? true : false;
    std::wstring str = GetIniString(cfgPath, L"image", L"posType");
    if (str == L"")
        str = L"0";
    ImgPosMode = std::stoi(str);
    if (ImgPosMode < 0 || ImgPosMode > 3)
        ImgPosMode = 0;

    //加载图像 Load Image
    std::wstring imgPath = GetCurDllDir() + L"\\Image";
    if (FileIsExist(imgPath))
    {
        std::vector<std::wstring> fileList;
        EnumFiles(imgPath, L"*.png", fileList);
        EnumFiles(imgPath, L"*.jpg", fileList);

        if (fileList.size() == 0) {
            MessageBoxW(0, L"文件资源管理器背景目录没有文件，因此扩展不会有任何效果.", L"缺少图片目录", MB_ICONERROR);
            return true;
        }

        for (size_t i = 0; i < fileList.size(); i++)
        {
            BitmapGDI* bmp = new BitmapGDI;
            HDC hMemDC = CreateCompatibleDC(0);
            bmp->src = new Gdiplus::Bitmap(fileList[i].c_str());
            bmp->pMem = hMemDC;
            bmp->Size = { (LONG)bmp->src->GetWidth(), (LONG)bmp->src->GetHeight() };
            bmp->src->GetHBITMAP(0, &bmp->pBmp);
            SelectObject(hMemDC, bmp->pBmp);
            pBgBmp.push_back(bmp);
            /*非随机 只加载一张
            * Load only one image non randomly
            */
            if (!Random)
                break;
        }
    }
    else {
        MessageBoxW(0, L"文件资源管理器背景目录不存在，因此扩展不会有任何效果.", L"缺少图片目录", MB_ICONERROR);
        return true;
    }
    //创建钩子 CreateHook
    if (MH_Initialize() == MH_OK)
    {
        CreateMHook(CreateWindowExW, MyCreateWindowExW, _CreateWindowExW_, 1);
        CreateMHook(DestroyWindow, MyDestroyWindow, _DestroyWindow_, 2);
        CreateMHook(BeginPaint, MyBeginPaint, _BeginPaint_, 3);
        CreateMHook(FillRect, MyFillRect, _FillRect_, 4);
        CreateMHook(CreateCompatibleDC, MyCreateCompatibleDC, _CreateCompatibleDC_, 4);
        MH_EnableHook(&CreateWindowExW);
        MH_EnableHook(&DestroyWindow);
        MH_EnableHook(&BeginPaint);
        MH_EnableHook(&FillRect);
        MH_EnableHook(&CreateCompatibleDC);
    }
    else
    {
        MessageBoxW(0, L"Failed to initialize disassembly!\nSuspected duplicate load extension", L"MTweaker Error", MB_ICONERROR | MB_OK);
        FreeLibraryAndExitThread(g_hModule, 0);
    }
    return true;
}

HWND MyCreateWindowExW(
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
)
{
    HWND hWnd = _CreateWindowExW_(dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    std::wstring ClassName;
    if (hWnd)
    {
        ClassName = GetWindowClassName(hWnd);
    }

    //explorer window 
    if (ClassName == L"DirectUIHWND"
        && GetWindowClassName(hWndParent) == L"SHELLDLL_DefView")
    {
        //继续查找父级 Continue to find parent
        HWND parent = GetParent(hWndParent);
        if (GetWindowClassName(parent) == L"ShellTabWindowClass")
        {
            //记录到列表中 Add to list
            MyData data;
            data.hWnd = hWnd;
            if (Random)
            {
                data.ImgIndex = rand() % ::pBgBmp.size();
            }
            DUIList.push_back(data);
        }
    }
    return hWnd;
}

BOOL MyDestroyWindow(HWND hWnd)
{
    //查找并删除列表中的记录 Find and remove from list
    for (size_t i = 0; i < DUIList.size(); i++)
    {
        if (DUIList[i].hWnd == hWnd)
        {
            //ReleaseDC(hWnd, DUIList[i].second);
            DUIList.erase(DUIList.begin() + i);
            break;
        }
    }
    return _DestroyWindow_(hWnd);
}

HDC MyBeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint)
{
    //开始绘制DUI窗口 BeginPaint dui window
    HDC hDC = _BeginPaint_(hWnd, lpPaint);
    for (size_t i = 0; i < DUIList.size(); i++)
    {
        if (DUIList[i].hWnd == hWnd)
        {
            //Log(L"Begin");
            //记录到列表 Record values to list
            DUIList[i].hDC = hDC;
            break;
        }
    }
    return hDC;
}

int MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr)
{
    int ret = _FillRect_(hDC, lprc, hbr);
    for (size_t i = 0; i < DUIList.size(); i++)
    {
        if (DUIList[i].hDC == hDC)
        {
            /*hbr = CreateSolidBrush(RGB(236, 64, 122));
            _FillRect_(hDC, lprc, hbr);
            DeleteObject(hbr);*/
            
            int size[2] = { lprc->right - lprc->left, lprc->bottom - lprc->top };
            RECT pRc;
            GetWindowRect(DUIList[i].hWnd, &pRc);
            SIZE wndSize = { pRc.right - pRc.left, pRc.bottom - pRc.top };

            /*因图片定位方式不同 如果窗口大小改变 需要全体重绘 否则有残留
            * Due to different image positioning methods,
            * if the window size changes, you need to redraw, otherwise there will be residues*/
            if ((DUIList[i].size.cx != wndSize.cx
                || DUIList[i].size.cy != wndSize.cy) && ImgPosMode != 0)
                InvalidateRect(DUIList[i].hWnd, 0, TRUE);

            /*裁剪矩形 Clip rect*/
            SaveDC(hDC);
            IntersectClipRect(hDC, lprc->left, lprc->top, lprc->right, lprc->bottom);

            BitmapGDI* pBgBmp = ::pBgBmp[DUIList[i].ImgIndex];

            //计算图片位置 Calculate picture position
            POINT pos;
            if (ImgPosMode == 0)
            {
                pos = { 0, 0 };
            }
            else if (ImgPosMode == 1)
            {
                pos.x = wndSize.cx - pBgBmp->Size.cx;
                pos.y = 0;
            }
            else if (ImgPosMode == 2)
            {
                pos.x = 0;
                pos.y = wndSize.cy - pBgBmp->Size.cy;
            }
            else if (ImgPosMode == 3)
            {
                pos.x = wndSize.cx - pBgBmp->Size.cx;
                pos.y = wndSize.cy - pBgBmp->Size.cy;
            }

            /*绘制图片 Paint image*/
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            AlphaBlend(hDC, pos.x, pos.y, pBgBmp->Size.cx, pBgBmp->Size.cy, pBgBmp->pMem, 0, 0, pBgBmp->Size.cx, pBgBmp->Size.cy, bf);

            RestoreDC(hDC, -1);

            DUIList[i].size = wndSize;

            //Log(L"DrawImage");
            return 0;
           // break;
        }
    }
    return ret;
}

HDC MyCreateCompatibleDC(HDC hDC)
{
    //在绘制DUI之前 会调用CreateCompatibleDC 找到它
    //CreateCompatibleDC is called before drawing the DUI
    HDC retDC = _CreateCompatibleDC_(hDC);
    for (size_t i = 0; i < DUIList.size(); i++)
    {
        if (DUIList[i].hDC == hDC)
        {
            DUIList[i].hDC = retDC;
            break;
        }
    }
    return retDC;
}