#include "App.h"
#include <assert.h>

App::App(HINSTANCE instance)
{
	this->instance = instance;
	this->device = nullptr;
	this->deviceContext = nullptr;
	this->frameBufferView = nullptr;
	this->depthStencilView = nullptr;
	this->windowResized = false;
}

/*virtual*/ App::~App()
{
}

bool App::Setup()
{
	if (!this->wormholeTree.LoadFromDisk("D:/git_repos/WormholeGenerator/Simulation/w0.wormhole"))
		return false;

	WNDCLASSEX winClass;
	::ZeroMemory(&winClass, sizeof(winClass));
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc = &App::WndProcEntryFunc;
	winClass.lpszClassName = WINDOW_CLASS_NAME;
	winClass.hIcon = NULL;

	if (!RegisterClassEx(&winClass))
		return false;

	RECT rect = { 0, 0, 1700, 800 };
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	this->windowHandle = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
											winClass.lpszClassName,
											TEXT("Wormhole Simulation"),
											WS_OVERLAPPEDWINDOW | WS_VISIBLE,
											CW_USEDEFAULT, CW_USEDEFAULT,
											width,
											height,
											0, 0, this->instance, 0);

	if (this->windowHandle == NULL)
		return false;

	SetWindowLongPtr(this->windowHandle, GWLP_USERDATA, LONG_PTR(this));

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;
	swapChainDesc.OutputWindow = this->windowHandle;

	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
													creationFlags, featureLevels, ARRAYSIZE(featureLevels),
													D3D11_SDK_VERSION, &swapChainDesc, &this->swapChain,
													&this->device, NULL, &this->deviceContext);

	if (FAILED(result))
		return false;

#if defined _DEBUG
	ID3D11Debug* debug = nullptr;
	device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
	if (debug)
	{
		ID3D11InfoQueue* infoQueue = nullptr;
		result = debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue);
		if (SUCCEEDED(result))
		{
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->Release();
		}
		debug->Release();
	}
#endif

	this->windowResized = false;

	if (!this->RecreateViews())
		return false;

	// The idea is to render the wormhole branches into off-screen textures that embed
	// both color and depth information, then composite those into the main frame buffer.
	// Before trying to do that, of course, just see if we can get one branch to render
	// into the main frame buffer directly.

	// STPTODO: Need to create sampler states.
	// STPTODO: Need to load pixel/vertex shaders.
	// STPTODO: Need to create vertex/index buffers for all nodes.

	return true;
}

bool App::Run(double deltaTime)
{
	bool keepRunning = true;

	MSG message{};
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
			keepRunning = false;

		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	if (this->windowResized)
	{
		this->deviceContext->OMSetRenderTargets(0, NULL, NULL);
		this->RecreateViews();
		this->windowResized = false;
	}

	this->Render();

	return keepRunning;
}

bool App::Shutdown()
{
	SafeRelease(this->device);
	SafeRelease(this->deviceContext);
	SafeRelease(this->swapChain);
	SafeRelease(this->frameBufferView);
	SafeRelease(this->depthStencilView);

	if (this->windowHandle)
	{
		DestroyWindow(this->windowHandle);
		this->windowHandle = nullptr;
	}

	UnregisterClass(WINDOW_CLASS_NAME, this->instance);

	return true;
}

void App::Render()
{
	FLOAT backgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	this->deviceContext->RSSetViewports(1, &this->viewport);
	this->deviceContext->ClearRenderTargetView(this->frameBufferView, backgroundColor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	this->deviceContext->OMSetRenderTargets(1, &this->frameBufferView, this->depthStencilView);
	
	//...

	this->swapChain->Present(1, 0);
}

bool App::RecreateViews()
{
	SafeRelease(this->frameBufferView);
	SafeRelease(this->depthStencilView);

	HRESULT result = 0;

	if (this->windowResized)
	{
		result = this->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(result))
			return false;
	}

	ID3D11Texture2D* backBufferTexture = nullptr;
	result = this->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);
	if (FAILED(result))
		return false;

	result = this->device->CreateRenderTargetView(backBufferTexture, NULL, &this->frameBufferView);
	if (FAILED(result))
		return false;

	D3D11_TEXTURE2D_DESC depthBufferDesc;
	backBufferTexture->GetDesc(&depthBufferDesc);
	backBufferTexture->Release();
	backBufferTexture = nullptr;

	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* depthBuffer = nullptr;
	result = this->device->CreateTexture2D(&depthBufferDesc, NULL, &depthBuffer);
	if (FAILED(result))
		return false;

	result = this->device->CreateDepthStencilView(depthBuffer, NULL, &this->depthStencilView);
	if (FAILED(result))
		return false;

	depthBuffer->Release();
	depthBuffer = nullptr;

	RECT clientRect;
	GetClientRect(this->windowHandle, &clientRect);

	this->viewport.TopLeftX = 0.0f;
	this->viewport.TopLeftY = 0.0f;
	this->viewport.Width = FLOAT(clientRect.right - clientRect.left);
	this->viewport.Height = FLOAT(clientRect.bottom - clientRect.top);
	this->viewport.MinDepth = 0.0f;
	this->viewport.MaxDepth = 1.0f;

	return true;
}

LRESULT App::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
				DestroyWindow(this->windowHandle);

			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_SIZE:
		{
			this->windowResized = true;
			break;
		}
	}

	return ::DefWindowProc(this->windowHandle, msg, wParam, lParam);
}

/*static*/ LRESULT App::WndProcEntryFunc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR userData = GetWindowLongPtr(windowHandle, GWLP_USERDATA);
	if (userData != 0)
	{
		auto app = (App*)userData;
		assert(app->windowHandle == windowHandle);
		return app->WndProc(msg, wParam, lParam);
	}

	return DefWindowProc(windowHandle, msg, wParam, lParam);
}