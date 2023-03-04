#include "d3d11_renderer.h"

#if defined(RENDERER_IMPL_D3D11)

#include "core.h"

D3D11Renderer* SpawnD3D11Renderer(HWND hwnd) {
	D3D11Renderer* renderer = new D3D11Renderer();
    renderer->type = RENDERER_D3D11;
	
	renderer->nativeWindow = hwnd;

	CreateDXGIFactory(__uuidof(IDXGIFactory), cast(void**, &renderer->factory));

	HRESULT deviceResult = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &renderer->device, NULL, &renderer->devcon);
	if (FAILED(deviceResult)) {
		// TODO: better error system than just asserts
		// - ... 2/5/2021
		Error("Unable to create DirectX11 device.");	    
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};	
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 1280; // TEMP: these 2 are hardcoded for now, TODO: add window get size
	swapChainDesc.BufferDesc.Height = 720; // 
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	HRESULT swapChainResult = renderer->factory->CreateSwapChain(renderer->device, &swapChainDesc, &renderer->swapChain);
	if (FAILED(swapChainResult)) 
	{
		Error("Unable to create DirectX11 swap chain.");	
	}


	
	// Get the texture that is needed to draw on the screen 
	ID3D11Texture2D* backBuffer;
	renderer->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), cast(void**, &backBuffer));

	renderer->device->CreateRenderTargetView(backBuffer, NULL, &renderer->renderTarget);
	backBuffer->Release(); 
	renderer->devcon->OMSetRenderTargets(1, &renderer->renderTarget, NULL);
	
	return renderer;
}

void DeleteD3D11Renderer(D3D11Renderer* renderer) {
	delete renderer;
}


void D3D11Renderer::TestRender() {
	float colors[4] = { 1, 0, 0, 1 };

	devcon->ClearRenderTargetView(renderTarget, colors);
	swapChain->Present(0, 0);
}


#endif
