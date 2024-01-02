#include "LapisEngine.h"
#include <iostream>
#include "Backend.h"

namespace Lapis
{

	float deltaTime;
	float elapsedTime;
	Transform mainCamera;


	void InitLapisInternal(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd)
	{
		std::cout << "hwnd: " << hwnd << "\n";
		Backend::InitBackendInternal(device, deviceContext, hwnd);
	}

	void NewFrame()
	{
		Backend::NewFrame();
	}
	void RenderFrame()
	{
		Backend::RenderFrame();
	}
	void FlushFrame()
	{
		Backend::FlushFrame();
	}
	void CleanLapis()
	{
		Backend::CleanD3D11();
	}


}