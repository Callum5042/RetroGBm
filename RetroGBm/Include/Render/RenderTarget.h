#pragma once

#include <d3d11_1.h>

// This include is requires for using DirectX smart pointers (ComPtr)
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

class Window;

namespace Render
{
	class RenderDevice;

	class RenderTarget
	{
		RenderDevice* m_RenderDevice = nullptr;
		Window* m_Window = nullptr;

	public:
		RenderTarget(RenderDevice* device);
		virtual ~RenderTarget() = default;

		void Create(Window* window);
		void Resize(int width, int height);

		void Clear();
		void Present();

	private:
		// Swapchain
		ComPtr<IDXGISwapChain> m_SwapChain = nullptr;
		ComPtr<IDXGISwapChain1> m_SwapChain1 = nullptr;
		void CreateSwapChain(int width, int height);

		// Render target and depth stencil view
		ComPtr<ID3D11RenderTargetView> m_RenderTargetView = nullptr;
		ComPtr<ID3D11DepthStencilView> m_DepthStencilView = nullptr;
		void CreateRenderTargetAndDepthStencilView(int width, int height);

		// Viewport
		void SetViewport(int width, int height);
		D3D11_VIEWPORT m_Viewport = {};
	};
}