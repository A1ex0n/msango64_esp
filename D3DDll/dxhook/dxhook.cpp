#include <windows.h>
#include <assert.h>
#include <d3d9.h>
#include "dxhook.h"
#include "../minhook/include/MinHook.h"

static LONG64* g_methodsTable = NULL;
LONG64 dx_init()
{
	WNDCLASSEXA windowClass;
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "DXHOOK";
	windowClass.hIconSm = NULL;
	::RegisterClassExA(&windowClass);
	HWND window = ::CreateWindowA(windowClass.lpszClassName, "DXHOOK WINDOW", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);
	HMODULE libD3D9;
	if ((libD3D9 = ::GetModuleHandleA("d3d9.dll")) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
		return FALSE;
	}

	void* Direct3DCreate9;
	if ((Direct3DCreate9 = ::GetProcAddress(libD3D9, "Direct3DCreate9")) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
		return FALSE;
	}

	LPDIRECT3D9 direct3D9;
	if ((direct3D9 = ((LPDIRECT3D9(__stdcall*)(LONG64))(Direct3DCreate9))(D3D_SDK_VERSION)) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
		return FALSE;
	}

	D3DDISPLAYMODE displayMode;
	if (direct3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
		return FALSE;
	}

	D3DPRESENT_PARAMETERS params;
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
	params.BackBufferFormat = displayMode.Format;
	params.BackBufferCount = 0;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality = NULL;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.Windowed = 1;
	params.EnableAutoDepthStencil = 0;
	params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	params.Flags = NULL;
	params.FullScreen_RefreshRateInHz = 0;
	params.PresentationInterval = 0;

	LPDIRECT3DDEVICE9 device;
	if (direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device) < 0)
	{
		direct3D9->Release();
		::DestroyWindow(window);
		::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
		return FALSE;
	}
	g_methodsTable = (LONG64*)::calloc(119, sizeof(LONG64));
	if (g_methodsTable==0)
	{
		return FALSE;
	}
	::memcpy(g_methodsTable, *(LONG64**)device, 119 * sizeof(LONG64));
	MH_Initialize();
	direct3D9->Release();
	direct3D9 = NULL;
	device->Release();
	device = NULL;
	::DestroyWindow(window);
	::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
	return TRUE;
}
void dx_shutdown()
{
	MH_DisableHook(MH_ALL_HOOKS);
	::free(g_methodsTable);
	g_methodsTable = NULL;
}
LONG64 dx_bind(LONG64 _index, void** _original, void* _function)
{
	void* target = (void*)g_methodsTable[_index];
	if (MH_CreateHook(target, _function, _original) != MH_OK || MH_EnableHook(target) != MH_OK)
	{
		return FALSE ;
	}
	return TRUE;
}
void dx_unbind(LONG64 _index)
{
	MH_DisableHook((void*)g_methodsTable[_index]);
}
LONG64* dx_getMethodsTable()
{
	return g_methodsTable;
} 