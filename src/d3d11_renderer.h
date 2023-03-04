#pragma once

#if defined(RENDERER_IMPL_D3D11)

#include "renderer.h"

#include <windows.h>
#include <d3d11.h>

struct D3D11Renderer : public Renderer {
	HWND nativeWindow;
	IDXGIFactory* factory;
	ID3D11Device* device;
	ID3D11DeviceContext* devcon;	
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* renderTarget;

	void TestRender() override;
};

D3D11Renderer* SpawnD3D11Renderer(HWND hwnd);
void DeleteD3D11Renderer(D3D11Renderer* renderer);

#endif
