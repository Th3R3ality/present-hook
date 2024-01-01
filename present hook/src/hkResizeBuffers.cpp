#include "hkResizeBuffers.h"
#include "hkPresent.h"

#include <iostream>

uintptr_t origResizeBuffers;
HRESULT hkResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    std::cout << "hkResizeBuffers\n";
    deviceContext->OMSetRenderTargets(0, 0, 0);
    renderTargetView->Release();
    renderTargetView = nullptr;

    auto hr = ((HRESULT(*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT))origResizeBuffers)(_this, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (FAILED(hr))
        return hr;

    ID3D11Texture2D* pBuffer;
    _this->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
    if (pBuffer)
        device->CreateRenderTargetView(pBuffer, NULL, &renderTargetView);
    pBuffer->Release();

    return hr;
}
