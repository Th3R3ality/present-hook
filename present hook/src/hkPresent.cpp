#include "hkPresent.h"
#include "global.h"

#include <iostream>
#include <format>
#include <d3d11.h>

#include "imgui-master/imgui.h"
#include "imgui-master/backends/imgui_impl_win32.h"
#include "imgui-master/backends/imgui_impl_dx11.h"

#include "Lapis/engine/LapisEngine.h"

ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
ID3D11RenderTargetView* renderTargetView;

ImGuiContext* imCtx;
WNDPROC origWndProcHandler;

HWND hwnd{};
bool ShowMenu = false;
bool doDebugPrint = false;

HRESULT hkPresent(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags)
{
	static int presentCalls = -1;
	presentCalls += 1;

	doDebugPrint = (presentCalls % 60 == 0);

	//std::cout << "hkPresent\n";
	//std::cout << std::format("Swapchain: {:#x}    SyncInterval: {}    Flags: {}\n", (uintptr_t)_this, SyncInterval, Flags);
	if (ejecting) {
		(WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)origWndProcHandler);

		presentReset = true;
		return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);
	}

	static bool init = false;
	if (!init) {
		std::cout << "initting\n";
		if (FAILED(GetDeviceAndCtxFromSwapchain(_this, &device, &deviceContext)))
			return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);


		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		_this->GetDesc(&swapChainDesc);
		hwnd = swapChainDesc.OutputWindow; // the hwnd
		std::cout << "hwnd : " << hwnd << "\n";

		ID3D11Texture2D* backBuffer = nullptr;
		_this->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
		if (!backBuffer) {
			return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);
		}
		device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
		backBuffer->Release();

		imCtx = ImGui::CreateContext();
		ImGui::SetCurrentContext(imCtx);

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(device, deviceContext);
		
		//Set OriginalWndProcHandler to the Address of the Original WndProc function
		origWndProcHandler = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)hWndProc);

		init = true;
		std::cout << "initted\n";
	}

	

	if ((GetAsyncKeyState(VK_INSERT) & 0x0001)) {
		ShowMenu = !ShowMenu;
		std::cout << "toggling : " << ShowMenu << "\n";
	}
	
	ImGui_ImplDX11_NewFrame();
	//std::cout << "dx11 newframe\n";
	ImGui_ImplWin32_NewFrame();
	//std::cout << "win32 newframe\n";
	ImGui::NewFrame();
	//std::cout << "imgui newframe\n";

	{
		ImGui::Begin(ShowMenu ? " + open + " : " - closed - ");
		//std::cout << "begin\n";
		ImGui::End();
		//std::cout << "end\n";
	}

	ImGui::Render();
	//std::cout << "imgui Render\n";
	deviceContext->OMSetRenderTargets(1, &renderTargetView, NULL);
	//std::cout << "omsetrendertargets\n";
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	//std::cout << "dx11 renderdrawdata\n";

	bool forceVSync = false;
	if (forceVSync)
		SyncInterval = 1; Flags = 0;
	return ((HRESULT(*)(IDXGISwapChain*, UINT, UINT))origPresent)(_this, SyncInterval, Flags);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//std::cout << "hooked WndProc\n";

	//ImGuiIO& io = ImGui::GetIO();
	//POINT mPos;
	//GetCursorPos(&mPos);
	//ScreenToClient(hwnd, &mPos);
	//ImGui::GetIO().MousePos.x = mPos.x;
	//ImGui::GetIO().MousePos.y = mPos.y;

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	if (ShowMenu) {
		//return true;
	}

	return CallWindowProc(origWndProcHandler, hWnd, uMsg, wParam, lParam);
}