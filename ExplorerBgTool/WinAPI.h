/*
* WinAPI声明
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#pragma once
#include <string>
#include <comdef.h>
#include <gdiplus.h>
#include <vector>

/*调试输出
* Debug output
*/
extern void Log(std::wstring log);
extern void Log(int log);

/*获取当前dll所在目录
* Get current directory
*/
extern std::wstring GetCurDllDir();

/*判断文件是否存在
* file exist
*/
extern bool FileIsExist(std::wstring FilePath);

/*获取窗口标题*/
extern std::wstring GetWindowTitle(HWND hWnd);

/*获取窗口类名*/
extern std::wstring GetWindowClassName(HWND hWnd);

/*读取配置文件内容
* Read config file
*/
extern std::wstring GetIniString(std::wstring FilePath, std::wstring AppName, std::wstring KeyName);

/*枚举某目录下指定文件*/
extern void EnumFiles(std::wstring path, std::wstring append, std::vector<std::wstring>& fileList);

/*取文件名*/
extern std::wstring GetFileName(std::wstring path);

/*GDI Bitmap*/
class BitmapGDI
{
public:
	BitmapGDI(std::wstring path);
	~BitmapGDI();

	HDC pMem = 0;
	HBITMAP pBmp = 0;
	SIZE Size;
	Gdiplus::Bitmap* src = 0;
};