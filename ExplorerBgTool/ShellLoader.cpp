/*
* BHO组件加载器
*
* Author: Maple
* date: 2022-1-31 Create
* Copyright winmoes.com
*/

#include "ShellLoader.h"

long g_cDllRef = 0;

/*如果您修改了代码 请使用VS的GUID工具生成新的GUID！*/
const std::wstring CLSID_SHELL_BHO_STR = L"{ED15A97D-FE3E-4CDE-98FF-BC46B02896B0}";
const CLSID CLSID_SHELL_BHO = { 0xed15a97d, 0xfe3e, 0x4cde, { 0x98, 0xff, 0xbc, 0x46, 0xb0, 0x28, 0x96, 0xb0 } };

#pragma region CObjectWithSite

CObjectWithSite::CObjectWithSite()
{
	OnWindowLoad();
}

CObjectWithSite::~CObjectWithSite()
{
	ReleaseRes();
}

STDMETHODIMP CObjectWithSite::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown)
		*ppv = static_cast<CObjectWithSite*>(this);
	else if (riid == IID_IObjectWithSite)
		*ppv = static_cast<IObjectWithSite*>(this);
	else
		return E_NOINTERFACE;
	AddRef();
	return S_OK;
}

ULONG __stdcall CObjectWithSite::AddRef()
{
	InterlockedIncrement(&g_cDllRef);
	return InterlockedIncrement(&m_ref);
}

ULONG __stdcall CObjectWithSite::Release()
{
	int tmp = InterlockedDecrement(&m_ref);
	if (tmp == 0)
		delete this;
	InterlockedDecrement(&g_cDllRef);
	return tmp;
}


STDMETHODIMP CObjectWithSite::SetSite(IUnknown* pUnkSite)
{
	ReleaseRes();
	HRESULT hr = pUnkSite->QueryInterface(IID_IWebBrowser2, (void**)&m_web);
	if (FAILED(hr))
		ReleaseRes();
	return hr;
}

void CObjectWithSite::ReleaseRes()
{
	if (m_web) m_web->Release();
	m_web = 0;
}

#pragma endregion

#pragma region ClassFactory

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IClassFactory)
	{ 
		*ppv = this;
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall ClassFactory::AddRef()
{
	InterlockedIncrement(&g_cDllRef);
	return InterlockedIncrement(&g_cDllRef);
}

ULONG __stdcall ClassFactory::Release()
{
	int tmp = InterlockedDecrement(&m_ref);
	if (tmp == 0)
		delete this;
	InterlockedDecrement(&g_cDllRef);
	return tmp;
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock)
{ 
	if (fLock)
		InterlockedIncrement(&g_cDllRef);
	else 
		InterlockedDecrement(&g_cDllRef);
	return S_OK;
}
STDMETHODIMP ClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObj)
{ 
	*ppvObj = NULL;
	if (pUnkOuter) 
		return CLASS_E_NOAGGREGATION;
	CObjectWithSite* bho = new CObjectWithSite();
	bho->AddRef();
	HRESULT hr = bho->QueryInterface(riid, ppvObj);
	bho->Release();
	return hr; 
}

#pragma endregion

#pragma region Reg

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppvOut)
{
	static ClassFactory factory;
	*ppvOut = NULL;
	if (rclsid == CLSID_SHELL_BHO) { 
		return factory.QueryInterface(riid, ppvOut);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
	return (g_cDllRef > 0) ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer()
{
	HKEY hkey = 0;
	TCHAR dllpath[MAX_PATH];
	GetModuleFileNameW(g_hModule, dllpath, MAX_PATH);

	//创建CLSID
	std::wstring regpath = L"CLSID\\" + CLSID_SHELL_BHO_STR;
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT,regpath.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//设置COM组件名称
	RegSetValueEx(hkey, NULL, 0, REG_SZ, (const BYTE*)L"CodeProject Example BHO", 24 * sizeof(TCHAR));
	RegCloseKey(hkey);

	//创建InProcServer32
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, (regpath + L"\\InProcServer32").c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//设置dll位置
	RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*)dllpath, (wcslen(dllpath) + 1) * sizeof(wchar_t));
	// Set the ThreadingModel to Apartment
	RegSetValueExW(hkey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)L"Apartment", 10 * sizeof(wchar_t));
	RegCloseKey(hkey);

	//注册BHO组件
	if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, (LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\)"
		+ CLSID_SHELL_BHO_STR).c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//禁止IE浏览器加载本组件
	DWORD value = 1;
	RegSetValueEx(hkey, L"NoInternetExplorer", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
	RegCloseKey(hkey);
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	//删除BHO组件注册
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\\)"
		+ CLSID_SHELL_BHO_STR).c_str());
	//删除COM组件注册
	RegDeleteKey(HKEY_CLASSES_ROOT, (L"CLSID\\" + CLSID_SHELL_BHO_STR + L"\\InProcServer32").c_str());
	RegDeleteKey(HKEY_CLASSES_ROOT, (L"CLSID\\" + CLSID_SHELL_BHO_STR).c_str());
	return S_OK;
}

#pragma endregion
