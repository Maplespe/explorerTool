/*
*  dll加载器实现
*
*  Author: Maple
*  date: 2021-7-25 Create
*  site: winmoes.com
*/

#include "framework.h"
#include "MToolLoader.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <Shlwapi.h>
#pragma comment(lib, "psapi.lib")
#pragma comment( lib, "shlwapi.lib")

/*
*  API助手声明
*
*  Author: Maple
*  date: 2021-7-26 Create
*  site: winmoes.com
*/
#pragma region Helper

std::wstring GetCurrentDir() {
    wchar_t sPath[MAX_PATH];
    GetModuleFileNameW(NULL, sPath, MAX_PATH);
    std::wstring path = sPath;
    path = path.substr(0, path.rfind(L"\\"));

    return path;
}

bool FileIsExist(std::wstring FilePath) {
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    hFind = FindFirstFileW(FilePath.c_str(), &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return true;
    }
    return false;
}

DWORD RegGetDWORD(HKEY hKey, LPCWSTR SubKey, LPCWSTR KeyName)
{
    DWORD ret = -1;
    HKEY tkey;
    if (ERROR_SUCCESS == RegOpenKeyExW(hKey, SubKey, 0, KEY_READ, &tkey))
    {
        DWORD dwValue;
        DWORD dwSzType = REG_DWORD;
        DWORD dwSize = sizeof(dwValue);
        if (RegQueryValueExW(tkey, KeyName, 0, &dwSzType, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        {
            dwValue = -1;
        }
        ret = dwValue;
    }
    RegCloseKey(tkey);
    return ret;
}

#pragma endregion

#pragma region Loader

bool InjectDLL(DWORD pid, std::wstring path, std::wstring& errmsg)
{
    DWORD threadSize = 512;
    DWORD writeSize = 0;
    LPVOID fun = LoadLibraryW;

    //打开进程
    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        errmsg = L"[Loader.cpp] " + MERROR_OpenProcess;
        return false;
    }

    //分配线程内存
    void* threadMem = VirtualAllocEx(proc, NULL, threadSize,
        MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!threadMem)
    {
        errmsg = L"[Loader.cpp] " + MERROR_AllocMemoryProc;
        CloseHandle(proc);
        return false;
    }

    //写入dll路径
    wchar_t* pathStr = new wchar_t[MAX_PATH];
    wcscpy_s(pathStr, MAX_PATH, path.c_str());
    if (!WriteProcessMemory(proc, threadMem, (LPVOID)pathStr,
        threadSize, NULL))
    {
        errmsg = L"[Loader.cpp] " + MERROR_WriteMemoryProc;
        delete[] pathStr;
        VirtualFreeEx(proc, threadMem, threadSize, MEM_COMMIT);
        CloseHandle(proc);
        return false;
    }

    //创建远程线程
    HANDLE remoteThread = CreateRemoteThread(proc, NULL, NULL,
        (LPTHREAD_START_ROUTINE)fun, threadMem, NULL, &writeSize);
    if (!remoteThread)
    {
        errmsg = L"[Loader.cpp] " + MERROR_CreateRemoteThread;
        delete[] pathStr;
        VirtualFreeEx(proc, threadMem, threadSize, MEM_COMMIT);
        CloseHandle(proc);
        return false;
    }

    //等待线程执行
    WaitForSingleObject(remoteThread, INFINITE);
    //清理
    delete[] pathStr;
    VirtualFreeEx(proc, threadMem, threadSize, MEM_COMMIT);
    CloseHandle(remoteThread);
    CloseHandle(proc);

    return true;
}

bool GetProcessByName(std::wstring name, std::vector<DWORD>& procInfo)
{
    //创建进程快照
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    //匹配名称
    if (!Process32FirstW(snapshot, &pe))
    {
        return false;
    }
    while (Process32NextW(snapshot, &pe))
    {
        if (!wcscmp(pe.szExeFile, name.c_str()))
        {
            procInfo.push_back(pe.th32ProcessID);
        }
    }

    return true;
}

bool IsDesktopMainThread(DWORD pid)
{
    struct EnumWindowArgs
    {
        int type;//标记类型
        DWORD ProcessID;//当前进程ID
        bool ret;//返回值
    };

    EnumWindowArgs args = { 0, pid, false };
    /*枚举窗口回调*/
    auto EnumWindowProc = [](HWND hWnd, LPARAM lParam) -> bool
    {
        DWORD ProcessId = NULL;
        GetWindowThreadProcessId(hWnd, &ProcessId);
        //取窗口类名
        wchar_t buffer[256];
        ZeroMemory(buffer, sizeof(wchar_t) * 256);
        GetClassNameW(hWnd, buffer, 256);
        std::wstring lpName = buffer;

        EnumWindowArgs* args = (EnumWindowArgs*)lParam;
        //判断是否为桌面进程
        if (lpName == L"Progman" && ProcessId == args->ProcessID && args->type == 0)
        {
            args->ret = true;
            return false;
        }
        return true;
    };

    EnumWindows((WNDENUMPROC)static_cast<bool(__stdcall*)
        (HWND, LPARAM)>(EnumWindowProc), (LPARAM)&args);
    return args.ret;
}

bool LoadMyToolDLL(std::wstring& errmsg)
{
    std::wstring myToolDLL = GetCurrentDir() + L"\\ExplorerBgTool.dll";
    if (!FileIsExist(myToolDLL))
    {
        errmsg = L"[Loader.cpp] " + MERROR_NotFoundExt;
        return false;
    }
    std::vector<DWORD> processList;
    if (!GetProcessByName(L"explorer.exe", processList))
    {
        errmsg = L"[Loader.cpp] " + MERROR_GetProcessList;
        return false;
    }
    bool separateProc = false;
    bool Injected = false;
    bool retry = false;
    /*判断Explorer进程类型
    * win10 1903起加入了 "在单独的进程中打开文件夹窗口"
    * 因此判断 是否为桌面进程 不注入桌面进程 他们是分开的
    */
    if (RegGetDWORD(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced)",
        L"SeparateProcess") == 1)
    {
        separateProc = true;
    }
Try:
    for (auto list : processList)
    {
        //多进程模式下 跳过桌面进程
        if (separateProc && IsDesktopMainThread(list))
        {
            continue;
        }

        /*枚举进程模块列表*/
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, list);
        DWORD need = 0;
        //取所需数组大小
        K32EnumProcessModules(hProcess, 0, 0, &need);
        //创建数组
        size_t size = need / sizeof(HMODULE);
        HMODULE* hModule = new HMODULE[size];
        if (!K32EnumProcessModules(hProcess, hModule, need, &need))
        {
            errmsg = L"[Loader.cpp] " + MERROR_EnumModules;
            CloseHandle(hProcess);
            delete[] hModule;
            return false;
        }
        for (size_t i = 0; i < size; i++) {
            //取模块路径
            wchar_t pStr[MAX_PATH];
            K32GetModuleFileNameExW(hProcess, hModule[i], pStr, MAX_PATH);
            //对比模块文件名
            std::wstring name = PathFindFileNameW(pStr);
            if (name.find(PathFindFileNameW(myToolDLL.c_str())) != -1)
            {
                errmsg = L"[Loader.cpp] " + MERROR_ModuleLoaded;
                delete[] hModule;
                CloseHandle(hProcess);
                return false;
            }
        }
        delete[] hModule;
        CloseHandle(hProcess);

        if (!InjectDLL(list, myToolDLL, errmsg))
            return false;
        Injected = true;
    }
    /*如果没有注入 因为找不到第二个文件资源管理器进程
    * 尝试创建一个explorer进程 然后重试一遍
    */
    if (!Injected)
    {
        if (!retry)
        {
            STARTUPINFO si = { sizeof(si) };
            PROCESS_INFORMATION pi = { 0 };
            wchar_t szCmd[] = L"explorer.exe";
            if (CreateProcessW(NULL, szCmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                //查找主窗口句柄
                HWND hWnd = 0;
                int time = 0;
                while (!hWnd)
                {
                    time++;
                    hWnd = FindWindowW(L"CabinetWClass", NULL);
                    if (hWnd)
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(hWnd, &pid);
                        if (pid == pi.dwProcessId)
                            break;
                    }
                    else
                        hWnd = 0;
                    //超过五秒都找不到文件资源管理器主窗口
                    if (time > 5000)
                    {
                        errmsg = L"[Loader.cpp] " + MERROR_ReTrytimeout;
                        return false;
                    }
                    Sleep(1);
                }
                //直接关闭窗口 不要显示出来
                ShowWindow(hWnd, SW_HIDE);
                SendMessageW(hWnd, WM_CLOSE, 0, 0);
                //重新读取进程
                processList.clear();
                if (!GetProcessByName(L"explorer.exe", processList))
                {
                    errmsg = L"[Loader.cpp] " + MERROR_GetProcessList;
                    return false;
                }
                retry = true;
                goto Try;
            }
            else
            {
                errmsg = L"[Loader.cpp] " + MERROR_CreateProcess;
                return false;
            }
        }
        else
        {
            errmsg += L"\n[Loader.cpp]" + MERROR_ReTryFailed;
            return false;
        }
    }

    return true;
}

#pragma endregion

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    std::wstring inCmd = lpCmdLine;
    if (inCmd != L"")
    {
        std::wstring err;
        if (inCmd == L"load" || inCmd == L"load_s") {
            if (LoadMyToolDLL(err)) {
                if(inCmd != L"load_s")
                    MessageBoxW(0, L"Loading succeeded! 加载成功!", L"Info", MB_ICONINFORMATION | MB_OK);
            }
            else
                MessageBoxW(0, err.c_str(), L"ERROR", MB_ICONERROR | MB_OK);
        }
        else if (inCmd == L"reload" || inCmd == L"unload")
        {
            auto cmd = "taskkill /f /im explorer.exe & start explorer.exe";
            system(cmd);
            Sleep(2000);
            if (inCmd == L"reload") {
                if (LoadMyToolDLL(err))
                    MessageBoxW(0, L"Loading succeeded! 加载成功!", L"Info", MB_ICONINFORMATION | MB_OK);
                else
                    MessageBoxW(0, err.c_str(), L"ERROR", MB_ICONERROR | MB_OK);
            }
        }
    }
    else
        MessageBoxW(0, L"Parameter error! 参数错误!", L"ERROR", MB_ICONERROR | MB_OK);

    return 0;
}