#pragma once
#include <Windows.h>
#include <d3d11.h>

#include "LapisTypes.h"
#include "Draw/D2.h"
#include "Draw/D3.h"


namespace Lapis
{

	extern float deltaTime;
	extern float elapsedTime;
	extern Transform mainCamera;


	void InitLapisInternal(IDXGISwapChain* swapchain);

	void WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void NewFrame();
	void RenderFrame();
	void FlushFrame();

	void CleanLapis();

	void DestroyViews();
	void CreateViews(IDXGISwapChain* swapchain);

}