#pragma once

#include <d3d11_1.h>
#include <DirectXMath.h>
using namespace DirectX;

// This include is requires for using DirectX smart pointers (ComPtr)
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

namespace Render
{
	class RenderDevice;

	struct CameraBuffer
	{
		XMMATRIX world;
		XMMATRIX camera_projection;
	};

	class RenderShader
	{
		RenderDevice* m_RenderDevice = nullptr;

	public:
		RenderShader(RenderDevice* device);
		virtual ~RenderShader() = default;

		void Create();

		// Bind the shader to the pipeline
		void Use();

		// Update the window size
		void UpdateSize(int width, int height, bool stretch);

		bool UseLinearFiltering = false;

	private:
		// Create vertex shader
		void LoadVertexShader();
		ComPtr<ID3D11VertexShader> m_VertexShader = nullptr;
		ComPtr<ID3D11InputLayout> m_VertexLayout = nullptr;

		// Create pixel shader
		void LoadPixelShader();
		ComPtr<ID3D11PixelShader> m_PixelShader = nullptr;

		// Sampler
		void CreatePointFilterSamplerState();
		ComPtr<ID3D11SamplerState> m_PointFilterSampler = nullptr;

		void CreateLinearFilterSamplerState();
		ComPtr<ID3D11SamplerState> m_LinearFilterSampler = nullptr;

		// Camera
		void CreateCameraConstantBuffer();
		ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;
	};
}