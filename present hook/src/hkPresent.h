#pragma once
#include <windows.h>
#include <WinUser.h>
#include <d3d11.h>

extern HWND g_hwnd;

extern uintptr_t origPresent;
HRESULT hkPresent(IDXGISwapChain* Swapchain, UINT SyncInterval, UINT Flags);

extern WNDPROC origWndProcHandler;
LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);