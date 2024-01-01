#pragma once

#include <d3d11.h>

extern uintptr_t origResizeBuffers;
HRESULT hkResizeBuffers(IDXGISwapChain* _this, 
	UINT BufferCount, 
	UINT Width, 
	UINT Height, 
	DXGI_FORMAT NewFormat, 
	UINT SwapChainFlags);