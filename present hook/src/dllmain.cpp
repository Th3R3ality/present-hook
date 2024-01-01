// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <chrono>
#include <thread>

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "global.h"
#include "Hooking.h"
#include "D3D_VMT_Indices.h"

#include "hkPresent.h"
#include "hkResizeBuffers.h"

bool ejecting;
bool presentReset;

HWND consoleHWND = nullptr;
void mainThread(HMODULE hModule);

uintptr_t origPresent{ 0 };


#define safe_release(p) if (p) { p->Release(); p = nullptr; }

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if ((consoleHWND = GetConsoleWindow()) == 0) {
            consoleHWND = (HWND)1;
            AllocConsole();
            FILE* f;
            freopen_s(&f, "CONOUT$", "w", stdout);
           std::cout << "Created Console\n";
        }
       else std::cout << "Console Present\n";

        if (auto handle = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(mainThread), hModule, 0, 0))
            CloseHandle(handle);

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void mainThread(HMODULE hModule)
{
    ejecting = false;
    presentReset = false;
    std::cout << "[+]\n";

    IDXGISwapChain* pSwapchain = nullptr;
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = GetForegroundWindow();                                // the window to be used
    scd.SampleDesc.Count = 4;                               // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, 
        D3D_DRIVER_TYPE_HARDWARE, 
        nullptr,
        0, 
        nullptr, 
        0, 
        D3D11_SDK_VERSION, 
        &scd, 
        &pSwapchain, 
        nullptr,
        nullptr,
        nullptr);
    
    std::cout << "D3D11CreateDeviceAndSwapChain : " << std::system_category().message(hr) << "\n";
    std::cout << "pSwapchain : " << pSwapchain << "\n";


    if (pSwapchain) {
        uintptr_t VMTaddr = *(uintptr_t*)pSwapchain;
        safe_release(pSwapchain);

        origPresent = VMTEntryHook(VMTaddr, (size_t)IDXGISwapChainVMT::Present, (uintptr_t)hkPresent);
        origResizeBuffers = VMTEntryHook(VMTaddr, (size_t)IDXGISwapChainVMT::ResizeBuffers, (uintptr_t)hkResizeBuffers);

        while (!GetAsyncKeyState(VK_DELETE))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ejecting = true;
        std::cout << "ejecting\n";
        while (!presentReset) { if (GetAsyncKeyState('L')) { presentReset = true; std::cout << "force ejecting\n"; } }

        VMTEntryHook(VMTaddr, (size_t)IDXGISwapChainVMT::Present, origPresent);
        VMTEntryHook(VMTaddr, (size_t)IDXGISwapChainVMT::ResizeBuffers, origResizeBuffers);
    }
    std::cout << "[-]\n\n";
    if (consoleHWND == 0)
        FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);

}