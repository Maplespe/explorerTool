/*
* 文件资源管理器背景工具扩展
* 
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <time.h>

//GDI 相关 Using GDI
#include <comdef.h>
#include <gdiplus.h>
#pragma comment(lib, "GdiPlus.lib")

#include "MinHook.h"
#include "ShellLoader.h"
#include "WinAPI.h"
#include "HookDef.h"

//AlphaBlend
#pragma comment(lib, "Msimg32.lib")  

using namespace Gdiplus;

//全局变量
#pragma region GlobalVariable

HMODULE g_hModule = NULL;           //全局模块句柄 Global module handle
bool m_isInitHook = false;          //Hook初始化标志 Hook init flag

ULONG_PTR m_gdiplusToken;           //GDI初始化标志 GDI Init flag

struct MyData
{
    HWND hWnd = 0;
    HDC hDC = 0;
    SIZE size = { 0,0 };
    int ImgIndex = 0;
};
//First ThreadID
std::unordered_map<DWORD, MyData> m_duiList;//dui句柄列表 dui handle list

struct imgInfo
{
    std::wstring fileName;
    BitmapGDI* bmp;
};

struct Config
{
    /* 0 = Left top
    *  1 = Right top
    *  2 = Left bottom
    *  3 = Right bottom
    *  4 = Center
    *  5 = Zoom
    *  6 = Zoom Fill
    */
    int imgPosMode = 0;                 //图片定位方式 Image position mode type
    bool isRandom = true;               //随机显示图片 Random pictures
    bool isCustom = false;              //自定义文件夹图片
    BYTE imgAlpha = 255;                //图片透明度 Image alpha
    std::vector<imgInfo> imageList;    //背景图列表 background image list
} m_config;                             //配置信息 config

#pragma endregion

extern bool InjectionEntryPoint();      //注入入口点
extern void LoadSettings(bool loadimg); //加载dll设置

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH && !g_hModule) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        //防止别的程序意外加载
        wchar_t pName[MAX_PATH];
        GetModuleFileNameW(NULL, pName, MAX_PATH);

        //进程名转小写
        std::wstring path = std::wstring(pName);
        std::wstring name = path.substr(path.length() - 12, 12);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == L"explorer.exe")
            InjectionEntryPoint();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        for (auto& info : m_config.imageList)
        {
            delete info.bmp;
        }
        m_config.imageList.clear();
        m_duiList.clear();
    }
    return TRUE;
}

bool InjectionEntryPoint()
{
    //设置随机数种子
    srand((int)time(0));

    return true;
}

void LoadSettings(bool loadimg)
{

    //释放旧资源
    if (loadimg) {
        for (auto& info : m_config.imageList)
        {
            delete info.bmp;
        }
        m_config.imageList.clear();
    }

    //加载配置 Load config
    std::wstring cfgPath = GetCurDllDir() + L"\\config.ini";
    m_config.isRandom = GetIniString(cfgPath, L"image", L"random") == L"true" ? true : false;
    m_config.isCustom = GetIniString(cfgPath, L"image", L"custom") == L"true" ? true : false;

    //图片定位方式
    std::wstring str = GetIniString(cfgPath, L"image", L"posType");
    if (str == L"") str = L"0";
    m_config.imgPosMode = std::stoi(str);
    if (m_config.imgPosMode < 0 || m_config.imgPosMode > 6)
        m_config.imgPosMode = 3;

    //图片透明度
    str = GetIniString(cfgPath, L"image", L"imgAlpha");
    if (str == L"")
        m_config.imgAlpha = 255;
    else
    {
        int alpha = std::stoi(str);
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        m_config.imgAlpha = (BYTE)alpha;
    }

    //加载图像 Load Image
    if (loadimg) {
        std::wstring imgPath = GetCurDllDir() + L"\\Image";
        if (FileIsExist(imgPath))
        {
            std::vector<std::wstring> fileList;
            EnumFiles(imgPath, L"*.png", fileList);
            EnumFiles(imgPath, L"*.jpg", fileList);

            if (fileList.size() == 0) {
                MessageBoxW(0, L"文件资源管理器背景目录没有文件，因此扩展不会有任何效果.", L"缺少图片文件", MB_ICONERROR);
                return;
            }

            for (size_t i = 0; i < fileList.size(); i++)
            {
                BitmapGDI* bmp = new BitmapGDI(fileList[i]);
                if (bmp->src)
                {
                    m_config.imageList.push_back({ GetFileName(fileList[i]), bmp });
                }
                else
                    delete bmp;//图片加载失败 load failed

                /*非随机 只加载一张
                * Load only one image non randomly
                */
                if (!m_config.isRandom && !m_config.isCustom) break;
            }
        }
        else {
            MessageBoxW(0, L"文件资源管理器背景目录不存在，因此扩展不会有任何效果.", L"缺少图片目录", MB_ICONERROR);
            return;
        }
    }
}

/*
* ShellLoader
* 文件资源管理器创建窗口时会调用 我们可以在这里更新一下配置
*/
void OnWindowLoad()
{
    //如果按住ESC键则不加载扩展
    if (GetKeyState(VK_ESCAPE) < 0)
        return;

    //在开机的时候系统就会加载此动态库 那时候启用HOOK是会失败的 等创建窗口的时候再初始化HOOK
    if (!m_isInitHook)
    {
        //初始化 Gdiplus Init GdiPlus
        GdiplusStartupInput StartupInput;
        int ret = GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);

        //创建钩子 CreateHook
        if (MH_Initialize() == MH_OK)
        {
            CreateMHook(CreateWindowExW, MyCreateWindowExW, _CreateWindowExW_, 1);
            CreateMHook(DestroyWindow, MyDestroyWindow, _DestroyWindow_, 2);
            CreateMHook(BeginPaint, MyBeginPaint, _BeginPaint_, 3);
            CreateMHook(FillRect, MyFillRect, _FillRect_, 4);
            CreateMHook(CreateCompatibleDC, MyCreateCompatibleDC, _CreateCompatibleDC_, 5);
            MH_EnableHook(MH_ALL_HOOKS);
        }
        else
        {
            MessageBoxW(0, L"Failed to initialize disassembly!\nSuspected duplicate load extension", L"MTweaker Error", MB_ICONERROR | MB_OK);
            FreeLibraryAndExitThread(g_hModule, 0);
        }
        m_isInitHook = true;
    }
    LoadSettings(true);
}

/*
* ShellLoader
* 文件资源管理器页面加载完毕后会调用
*/
void OnDocComplete(std::wstring path, DWORD threadID)
{
    //std::wcout << L"path[" << threadID << L"] " << path << L"\n";
    if (m_config.isCustom)
    {
        std::wstring fileName = GetIniString(GetCurDllDir() + L"\\config.ini", path, L"img");
        if (fileName.empty())
            return;
        auto size = m_config.imageList.size();
        for (size_t i = 0; i < size; i++)
        {
            if (m_config.imageList[i].fileName == fileName)
            {
                m_duiList[threadID].ImgIndex = i;
                break;
            }
        }
    }
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
            auto imgSize = m_config.imageList.size();
            if (m_config.isRandom && imgSize)
            {
                if (m_config.isRandom)
                    data.ImgIndex = rand() % imgSize;
                else
                    data.ImgIndex = 0;
            }
            m_duiList[GetCurrentThreadId()] = data;
        }
    }
    return hWnd;
}

BOOL MyDestroyWindow(HWND hWnd)
{
    //查找并删除列表中的记录 Find and remove from list
    auto iter = m_duiList.find(GetCurrentThreadId());
    if (iter != m_duiList.end())
    {
        if (iter->second.hWnd == hWnd)
            m_duiList.erase(iter);
    }
    return _DestroyWindow_(hWnd);
}

HDC MyBeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint)
{
    //开始绘制DUI窗口 BeginPaint dui window
    HDC hDC = _BeginPaint_(hWnd, lpPaint);

    auto iter = m_duiList.find(GetCurrentThreadId());

    if (iter != m_duiList.end()) {
        if (iter->second.hWnd == hWnd)
        {
            //Log(L"Begin");
            //记录到列表 Record values to list
            iter->second.hDC = hDC;
        }
    }
    return hDC;
}

int MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr)
{
    int ret = _FillRect_(hDC, lprc, hbr);

    auto iter = m_duiList.find(GetCurrentThreadId());
    if (iter != m_duiList.end()) {
        if (iter->second.hDC == hDC && m_config.imageList.size())
        {
            RECT pRc;
            GetWindowRect(iter->second.hWnd, &pRc);
            SIZE wndSize = { pRc.right - pRc.left, pRc.bottom - pRc.top };

            /*因图片定位方式不同 如果窗口大小改变 需要全体重绘 否则有残留
            * Due to different image positioning methods,
            * if the window size changes, you need to redraw, otherwise there will be residues*/
            if ((iter->second.size.cx != wndSize.cx || iter->second.size.cy != wndSize.cy)
                && m_config.imgPosMode != 0) {
                InvalidateRect(iter->second.hWnd, 0, TRUE);
            }

            //裁剪矩形 Clip rect
            SaveDC(hDC);
            IntersectClipRect(hDC, lprc->left, lprc->top, lprc->right, lprc->bottom);

            BitmapGDI* pBgBmp = m_config.imageList[iter->second.ImgIndex].bmp;

            //计算图片位置 Calculate picture position
            POINT pos;
            SIZE dstSize = { pBgBmp->Size.cx, pBgBmp->Size.cy };
            switch (m_config.imgPosMode)
            {
            case 0://左上
                pos = { 0, 0 };
                break;
            case 1://右上
                pos.x = wndSize.cx - pBgBmp->Size.cx;
                pos.y = 0;
                break;
            case 2://左下
                pos.x = 0;
                pos.y = wndSize.cy - pBgBmp->Size.cy;
                break;
            case 3://右下
                pos.x = wndSize.cx - pBgBmp->Size.cx;
                pos.y = wndSize.cy - pBgBmp->Size.cy;
                break;
            case 4://居中正常顯示
                pos.x = (wndSize.cx - pBgBmp->Size.cx) >> 1;
                pos.y = (wndSize.cy - pBgBmp->Size.cy) >> 1;
                break;
            case 5://缩放
                  {
                      int newWidth = wndSize.cx;
                      int newHeight = wndSize.cy;
                      pos.x = 0;
                      pos.y = 0;

                      dstSize = { newWidth, newHeight };
                  }
                  break;
            case 6://缩放并填充
                 {
                     static auto calcAspectRatio = [](int fromWidth, int fromHeight, int toWidthOrHeight, bool isWidth)
                     {
                         if (isWidth) {
                             return (int)round(((float)fromHeight * ((float)toWidthOrHeight / (float)fromWidth)));
                         }
                         else {
                             return (int)round(((float)fromWidth * ((float)toWidthOrHeight / (float)fromHeight)));
                         }
                     };

                     //按高等比例拉伸
                     int newWidth = calcAspectRatio(pBgBmp->Size.cx, pBgBmp->Size.cy, wndSize.cy, false);
                     int newHeight = wndSize.cy;

                     pos.x = newWidth - wndSize.cx;
                     pos.x /= 2;//居中
                     if (pos.x != 0) pos.x = -pos.x;
                     pos.y = 0;

                     //按高不足以填充宽 按宽
                     if (newWidth < wndSize.cx) {
                         newWidth = wndSize.cx;
                         newHeight = calcAspectRatio(pBgBmp->Size.cx, pBgBmp->Size.cy, wndSize.cx, true);
                         pos.x = 0;
                         pos.y = newHeight - wndSize.cy;
                         pos.y /= 2;//居中
                         if (pos.y != 0) pos.y = -pos.y;
                     }
                     dstSize = { newWidth, newHeight };
                 }
                break;
            default://默認右下
                pos.x = wndSize.cx - pBgBmp->Size.cx;
                pos.y = wndSize.cy - pBgBmp->Size.cy;
                break;
            }

            /*绘制图片 Paint image*/
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, m_config.imgAlpha, AC_SRC_ALPHA };
            AlphaBlend(hDC, pos.x, pos.y, dstSize.cx, dstSize.cy, pBgBmp->pMem, 0, 0, pBgBmp->Size.cx, pBgBmp->Size.cy, bf);

            RestoreDC(hDC, -1);

            iter->second.size = wndSize;

            //Log(L"DrawImage");
        }
    }
    return ret;
}

HDC MyCreateCompatibleDC(HDC hDC)
{
    //在绘制DUI之前 会调用CreateCompatibleDC 找到它
    //CreateCompatibleDC is called before drawing the DUI
    HDC retDC = _CreateCompatibleDC_(hDC);

    auto iter = m_duiList.find(GetCurrentThreadId());
    if (iter != m_duiList.end()) {
        if (iter->second.hDC == hDC)
            iter->second.hDC = retDC;
    }
    return retDC;
}
