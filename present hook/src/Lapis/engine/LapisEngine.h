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


	void InitLapisInternal(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd);

	void NewFrame();
	void RenderFrame();
	void FlushFrame();

	void CleanLapis();
}