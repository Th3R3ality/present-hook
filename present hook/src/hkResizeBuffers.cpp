#include "hkResizeBuffers.h"
#include "hkPresent.h"

#include <iostream>
#include "global.h"

#include "Lapis/engine/LapisEngine.h"

uintptr_t origResizeBuffers;
HRESULT hkResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    std::cout << "hkResizeBuffers\n";


    Lapis::DestroyViews();

    auto hr = ((HRESULT(*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT))origResizeBuffers)(_this, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (FAILED(hr))
        return hr;


    Lapis::CreateViews(_this);

    return hr;
}
