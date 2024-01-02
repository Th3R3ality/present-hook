#include "hkPresent.h"
#include "global.h"

#include <iostream>
#include <format>
#include <d3d11.h>

#include "Lapis/engine/LapisEngine.h"

HWND g_hwnd{};

WNDPROC origWndProcHandler{};

bool ShowMenu = false;
bool doDebugPrint = false;

HRESULT hkPresent(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags)
{
	static int presentCalls = -1;
	presentCalls += 1;

	doDebugPrint = (presentCalls % 60 == 0);

	if (ejecting) {
		Lapis::CleanLapis();
		if (origWndProcHandler != nullptr)
			(WNDPROC)SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)origWndProcHandler);

		presentReset = true;
		return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);
	}

	static bool init = false;
	if (!init) {
		std::cout << "initting from hkPresent\n";

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		_this->GetDesc(&swapChainDesc);
		g_hwnd = swapChainDesc.OutputWindow; // the g_hwnd
		std::cout << "g_hwnd : " << g_hwnd << "\n";

		Lapis::InitLapisInternal(_this);

		//Set OriginalWndProcHandler to the Address of the Original WndProc function
		origWndProcHandler = (WNDPROC)SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hWndProc);

		init = true;
		std::cout << "initted\n";
	}


	// lapis render
	{
		using namespace Lapis;
		Lapis::NewFrame();

		Lapis::Draw::D2::Triangle(100, { 500,100 }, 300, { 0.5,0.5,0.5,0.5 });

		Lapis::Draw::D3::Cube(Transform(Vec3::forward*3 + -Vec3::up, 0, 1), { 0.92, 0.26, .27, 1 });

		Lapis::RenderFrame();
		Lapis::FlushFrame();
	}

	return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);
}

LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Lapis::WndProcHandler(hWnd, uMsg, wParam, lParam);

	return CallWindowProc(origWndProcHandler, hWnd, uMsg, wParam, lParam);
}