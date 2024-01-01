#pragma once
#include <windows.h>
#include <WinUser.h>
#include <d3d11.h>

extern ID3D11Device* device;
extern ID3D11DeviceContext* deviceContext;
extern ID3D11RenderTargetView* renderTargetView;

extern uintptr_t origPresent;
HRESULT hkPresent(IDXGISwapChain* Swapchain, UINT SyncInterval, UINT Flags);

extern WNDPROC origWndProcHandler;
LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


inline HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);

	if (SUCCEEDED(ret))
		(*ppDevice)->GetImmediateContext(ppContext);

	return ret;
}