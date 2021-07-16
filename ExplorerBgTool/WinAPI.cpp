/*
* WinAPI声明
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#include "pch.h"
#include "framework.h"
#include "WinAPI.h"
#include <io.h>

void Log(std::wstring log)
{
	OutputDebugStringW((L"\n[Debug]: " + log).c_str());
}

void Log(int log)
{
	Log(std::to_wstring(log));
}

std::wstring GetCurDllDir()
{
	wchar_t sPath[MAX_PATH];
	GetModuleFileNameW(g_hModule, sPath, MAX_PATH);
	std::wstring path = sPath;
	path = path.substr(0, path.rfind(L"\\"));

	return path;
}

bool FileIsExist(std::wstring FilePath)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFileW(FilePath.c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		return true;
	}
	return false;
}

std::wstring GetWindowTitle(HWND hWnd)
{
	int size = GetWindowTextLengthW(hWnd);
	wchar_t* pText = new wchar_t[size];
	GetWindowTextW(hWnd, pText, size);
	return std::wstring(pText);
}

std::wstring GetWindowClassName(HWND hWnd)
{
	wchar_t* pText = new wchar_t[MAX_PATH];
	GetClassNameW(hWnd, pText, MAX_PATH);
	return std::wstring(pText);
}

std::wstring GetIniString(std::wstring FilePath, std::wstring AppName, std::wstring KeyName)
{
	if (FileIsExist(FilePath)) {
		HANDLE pFile = CreateFileW(FilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		LARGE_INTEGER fileSize;
		GetFileSizeEx(pFile, &fileSize);

		wchar_t* data = new wchar_t[fileSize.QuadPart];
		ZeroMemory(data, sizeof(wchar_t) * fileSize.QuadPart);
		GetPrivateProfileStringW(AppName.c_str(), KeyName.c_str(), NULL, data, fileSize.QuadPart, FilePath.c_str());

		std::wstring ret = data;
		delete[] data;

		CloseHandle(pFile);
		return ret;
	}
	return std::wstring();
}

void EnumFiles(std::wstring path, std::wstring append, std::vector<std::wstring>& fileList)
{
	//文件句柄 
	intptr_t  hFile = 0;
	//文件信息 
	struct _wfinddata_t fileinfo;
	std::wstring p;
	if ((hFile = _wfindfirst(p.assign(path).append(L"\\" + append).c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				std::wstring path_ = path + L"\\";
				path_ += fileinfo.name;
				fileList.push_back(path_);
			}
		} while (_wfindnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}
