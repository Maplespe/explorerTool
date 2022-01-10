/*
*  dll加载器声明
*
*  Author: Maple
*  date: 2021-7-25 Create
*  site: winmoes.com
*/
#pragma once
#include "framework.h"

/*以下代码部分复制自项目MyToolBox ErrorDef.h
* Author: Maple
* 2021-7-25 Create
* 仅供查询参考
*/
#pragma region ERROR_DEF

#define errstr const std::wstring

/*此错误(0x00) 代表缺少扩展文件ExplorerBgTool.dll*/
errstr MERROR_NotFoundExt = L"ERROR(0x00) dllfile Not Found!";

/*以下错误(0x01-0x07) 即进程无法打开、内存无法写入进程、无法远程加载线程
* 请检查软件是否有足够的权限 或是否被杀软阻止操作*/
errstr MERROR_OpenProcess = L"MERROR(0x01) OpenProcess failed!";
errstr MERROR_AllocMemoryProc = L"MERROR(0x02) Alloc process memory failed!";
errstr MERROR_WriteMemoryProc = L"MERROR(0x03) WriteProcess memory failed!";
errstr MERROR_CreateRemoteThread = L"MERROR(0x04) CreateRemoteThread failed!";
errstr MERROR_CreateProcess = L"MERROR(0x05) CreateProcess failed!";

errstr MERROR_GetProcessList = L"MERROR(0x06) GetProcessList failed!";
errstr MERROR_EnumModules = L"MERROR(0x07) EnumModules failed!";

//此错误(0x08) 代表程序的扩展文件已经加载 不能重复加载
errstr MERROR_ModuleLoaded = L"MERROR(0x08) Module loaded!";

/*此错误(0x09) 代表程序在加载工具扩展时失败 并创建新进程重试 但依然失败
* 请检查应用是否有权限读取进程列表和创建进程的权限 或是否有应用阻止了explorer的创建
*/
errstr MERROR_ReTryFailed = L"MERROR(0x09) Retry LoadTool failed!";

/*此错误(0x10) 代表程序在重试加载工具扩展时超时(5s) 无法找到文件资源管理器窗口
* 请检查是否有应用阻止了explorer创建窗口
*/
errstr MERROR_ReTrytimeout = L"MERROR(0x10) Retry timeout loadtool failed!";

#pragma endregion

/*注入dll
* @param pid - 进程ID
* @param path - dll库文件路径
* @param out errmsg - 返回消息
*
* @return 是否成功
*/
extern bool InjectDLL(DWORD pid, std::wstring path, std::wstring& errmsg);

/*取指定进程列表自名称
* @param name - 进程名称
* @param out procInfo - 进程ID数组
*
* @return 是否成功
*/
extern bool GetProcessByName(std::wstring name, std::vector<DWORD>& procInfo);

/*加载扩展*/
extern bool LoadMyToolDLL(std::wstring& errmsg);