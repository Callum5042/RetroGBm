#pragma once

#include <memory>
#include <d3d11_1.h>

// This include is requires for using DirectX smart pointers (ComPtr)
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

namespace Render
{
	class RenderTarget;
	class RenderTexture;

	class RenderDevice
	{
	public:
		RenderDevice() = default;
		virtual ~RenderDevice() = default;

		void Create();

		inline ComPtr<ID3D11Device> GetDevice() { return m_Device; }
		inline ComPtr<ID3D11DeviceContext> GetDeviceContext() { return m_DeviceContext; }

		// Create target
		std::unique_ptr<Render::RenderTarget> CreateRenderTarget();

		// Create shader


		// Create texture
		std::unique_ptr<Render::RenderTexture> CreateTexture();

	private:
		// Device and device context
		ComPtr<ID3D11Device> m_Device = nullptr;
		ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
		void CreateDeviceAndContext();
	};
}