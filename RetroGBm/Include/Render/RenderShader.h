#pragma once

#include <d3d11_1.h>

// This include is requires for using DirectX smart pointers (ComPtr)
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

namespace Render
{
	class RenderDevice;

	class RenderShader
	{
		RenderDevice* m_RenderDevice = nullptr;

	public:
		RenderShader(RenderDevice* device);
		virtual ~RenderShader() = default;

		void Create();

		// Bind the shader to the pipeline
		void Use();

	private:
		// Create vertex shader
		void LoadVertexShader();
		ComPtr<ID3D11VertexShader> m_VertexShader = nullptr;
		ComPtr<ID3D11InputLayout> m_VertexLayout = nullptr;

		// Create pixel shader
		void LoadPixelShader();
		ComPtr<ID3D11PixelShader> m_PixelShader = nullptr;

		// Sampler
		ComPtr<ID3D11SamplerState> m_AnisotropicSampler = nullptr;
	};
}