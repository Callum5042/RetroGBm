#include "Renderer.h"
#include <DirectXColors.h>
#include <iostream>
#include <vector>

DX::Renderer::Renderer(Window* window) : m_Window(window)
{
}

void DX::Renderer::Create()
{
	// Query window size
	auto window_width = 0;
	auto window_height = 0;
	m_Window->GetSize(&window_width, &window_height);

	// Setup Direct3D 11
	CreateDeviceAndContext();
	CreateSwapChain(window_width, window_height);
	CreateRenderTargetAndDepthStencilView(window_width, window_height);
	SetViewport(window_width, window_height);
}

void DX::Renderer::Resize(int width, int height)
{
	// Releases the current render target and depth stencil view
	m_d3dDepthStencilView.ReleaseAndGetAddressOf();
	m_d3dRenderTargetView.ReleaseAndGetAddressOf();

	// Resize the swapchain
	DX::Check(m_d3dSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

	// Creates a new render target and depth stencil view with the new window size
	CreateRenderTargetAndDepthStencilView(width, height);

	// Sets a new viewport with the new window size
	SetViewport(width, height);
}

void DX::Renderer::Clear()
{
	// Clear the render target view to the chosen colour
	m_d3dDeviceContext->ClearRenderTargetView(m_d3dRenderTargetView.Get(), reinterpret_cast<const float*>(&DirectX::Colors::SteelBlue));
	m_d3dDeviceContext->ClearDepthStencilView(m_d3dDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Bind the render target view to the pipeline's output merger stage
	m_d3dDeviceContext->OMSetRenderTargets(1, m_d3dRenderTargetView.GetAddressOf(), m_d3dDepthStencilView.Get());
}

void DX::Renderer::Present()
{
	// Check if we support IDXGISwapChain1
	if (m_d3dSwapChain1 != nullptr)
	{
		// Use IDXGISwapChain1::Present1 for presenting instead
		// This is a requirement for using variable refresh rate displays
		DXGI_PRESENT_PARAMETERS presentParameters = {};
		DX::Check(m_d3dSwapChain1->Present1(1, 0, &presentParameters));
	}
	else
	{
		DX::Check(m_d3dSwapChain->Present(0, 0));
	}
}

void DX::Renderer::CreateDeviceAndContext()
{
	// Look for Direct3D 11 feature
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// Add debug flag if in debug mode
	D3D11_CREATE_DEVICE_FLAG deviceFlag = (D3D11_CREATE_DEVICE_FLAG)0;
#ifdef _DEBUG
	deviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create device and device context
	D3D_FEATURE_LEVEL featureLevel;
	DX::Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlag, featureLevels, numFeatureLevels,
			  D3D11_SDK_VERSION, m_d3dDevice.ReleaseAndGetAddressOf(), &featureLevel, m_d3dDeviceContext.ReleaseAndGetAddressOf()));

	// Check if Direct3D 11.1 is supported, if not fall back to Direct3D 11
	if (featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		MessageBox(NULL, L"D3D_FEATURE_LEVEL_11_1 is not supported! Falling back to D3D_FEATURE_LEVEL_11_0", L"Error", MB_OK);
	}

	// Check if Direct3D 11 is supported
	if (featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		MessageBox(NULL, L"D3D_FEATURE_LEVEL_11_0 is not supported", L"Error", MB_OK);
		throw std::exception();
	}
}

void DX::Renderer::CreateSwapChain(int width, int height)
{
	// Get the Win32 window from SDL_Window
	auto hwnd = m_Window->GetHwnd();

	// Query the device until we get the DXGIFactory
	ComPtr<IDXGIDevice> dxgiDevice = nullptr;
	DX::Check(m_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> adapter = nullptr;
	DX::Check(dxgiDevice->GetAdapter(adapter.GetAddressOf()));

	ComPtr<IDXGIFactory> dxgiFactory = nullptr;
	DX::Check(adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())));

	// Query IDXGIFactory to try to get IDXGIFactory2
	ComPtr<IDXGIFactory2> dxgiFactory2 = nullptr;
	DX::Check(dxgiFactory.As(&dxgiFactory2));

	// If we can support IDXGIFactory2 then use it to create the swap chain, otherwise fallback to IDXIFactory
	if (dxgiFactory2 != nullptr)
	{
		// DirectX 11.1
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.Width = width;
		swapchain_desc.Height = height;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.BufferCount = 2;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		// CreateSwapChainForHwnd is the prefered way of creating the swap chain
		DX::Check(dxgiFactory2->CreateSwapChainForHwnd(m_d3dDevice.Get(), hwnd, &swapchain_desc, nullptr, nullptr, &m_d3dSwapChain1));
		DX::Check(m_d3dSwapChain1.As(&m_d3dSwapChain));
	}
	else
	{
		// Describe the swapchain
		DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
		swapchain_desc.BufferDesc.Width = width;
		swapchain_desc.BufferDesc.Height = height;
		swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.BufferCount = 2;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
		swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
		swapchain_desc.OutputWindow = hwnd;
		swapchain_desc.Windowed = TRUE;

		// Creates the swapchain
		DX::Check(dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &swapchain_desc, &m_d3dSwapChain));
	}
}

void DX::Renderer::CreateRenderTargetAndDepthStencilView(int width, int height)
{
	// Create the render target view
	ComPtr<ID3D11Texture2D> back_buffer = nullptr;
	DX::Check(m_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(back_buffer.GetAddressOf())));
	DX::Check(m_d3dDevice->CreateRenderTargetView(back_buffer.Get(), nullptr, m_d3dRenderTargetView.GetAddressOf()));

	// Describe the depth stencil view
	D3D11_TEXTURE2D_DESC depth_desc = {};
	depth_desc.Width = width;
	depth_desc.Height = height;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_desc.SampleDesc.Count = 1;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	// Create the depth stencil view
	ComPtr<ID3D11Texture2D> depth_stencil = nullptr;
	DX::Check(m_d3dDevice->CreateTexture2D(&depth_desc, nullptr, &depth_stencil));
	DX::Check(m_d3dDevice->CreateDepthStencilView(depth_stencil.Get(), nullptr, m_d3dDepthStencilView.GetAddressOf()));

	// Binds both the render target and depth stencil to the pipeline's output merger stage
	m_d3dDeviceContext->OMSetRenderTargets(1, m_d3dRenderTargetView.GetAddressOf(), m_d3dDepthStencilView.Get());
}

void DX::Renderer::SetViewport(int width, int height)
{
	// Describe the viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	// Bind viewport to the pipline's rasterization stage
	m_d3dDeviceContext->RSSetViewports(1, &viewport);
}
