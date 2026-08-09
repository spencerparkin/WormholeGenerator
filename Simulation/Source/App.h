#pragma once

#include <Windows.h>
#include <d3d11.h>
#include "WormholeGenerator/WormholeTree.h"

template<typename T>
void SafeRelease(T*& thing)
{
	if (thing)
	{
		thing->Release();
		thing = nullptr;
	}
}

#define WINDOW_CLASS_NAME	TEXT("WormholeSimulationWindowClass")

class App
{
public:
	App(HINSTANCE instance);
	virtual ~App();

	bool Setup();
	bool Run(double deltaTime);
	bool Shutdown();

private:
	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK WndProcEntryFunc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam);

	bool RecreateViews();
	void Render();

	HINSTANCE instance;
	HWND windowHandle;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* frameBufferView;
	ID3D11DepthStencilView* depthStencilView;
	bool windowResized;
	D3D11_VIEWPORT viewport;
	WormholeGenerator::WormholeTree wormholeTree;
};