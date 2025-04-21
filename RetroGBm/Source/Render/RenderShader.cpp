#include "Render/RenderShader.h"
#include "Render/RenderDevice.h"

#include "../Shaders/PixelShader.hlsl.h"
#include "../Shaders/VertexShader.hlsl.h"

#include <RetroGBm/Constants.h>

namespace DX
{
	inline void Check(HRESULT hr)
	{
#ifdef _DEBUG
		if (FAILED(hr))
		{
			throw std::exception();
		}
#endif
	}
}

Render::RenderShader::RenderShader(RenderDevice* device) : m_RenderDevice(device)
{
}

void Render::RenderShader::Create()
{
	LoadPixelShader();
	LoadVertexShader();

	CreatePointFilterSamplerState();
	CreateLinearFilterSamplerState();

	CreateCameraConstantBuffer();

	// Set buffer
	UpdateSize(800, 600, false);
}

void Render::RenderShader::CreatePointFilterSamplerState()
{
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = 1000.0f;

	DX::Check(m_RenderDevice->GetDevice()->CreateSamplerState(&sampler_desc, &m_PointFilterSampler));
}

void Render::RenderShader::CreateLinearFilterSamplerState()
{
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = 1000.0f;

	DX::Check(m_RenderDevice->GetDevice()->CreateSamplerState(&sampler_desc, &m_LinearFilterSampler));
}

void Render::RenderShader::CreateCameraConstantBuffer()
{
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(CameraBuffer);
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::Check(m_RenderDevice->GetDevice()->CreateBuffer(&desc, nullptr, m_ConstantBuffer.ReleaseAndGetAddressOf()));
}

void Render::RenderShader::Use()
{
	// Bind the input layout to the pipeline's Input Assembler stage
	m_RenderDevice->GetDeviceContext()->IASetInputLayout(m_VertexLayout.Get());

	// Bind the vertex shader to the pipeline's Vertex Shader stage
	m_RenderDevice->GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);

	// Bind the pixel shader to the pipeline's Pixel Shader stage
	m_RenderDevice->GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);

	// Bind pixel shader texture sampler
	m_RenderDevice->GetDeviceContext()->PSSetSamplers(0, 1, (UseLinearFiltering ? m_LinearFilterSampler.GetAddressOf() : m_PointFilterSampler.GetAddressOf()));

	// Set constant buffer
	const int camera_buffer_slot = 0;
	m_RenderDevice->GetDeviceContext()->VSSetConstantBuffers(camera_buffer_slot, 1, m_ConstantBuffer.GetAddressOf());
	m_RenderDevice->GetDeviceContext()->PSSetConstantBuffers(camera_buffer_slot, 1, m_ConstantBuffer.GetAddressOf());
}

void Render::RenderShader::LoadVertexShader()
{
	// Create the vertex shader
	DX::Check(m_RenderDevice->GetDevice()->CreateVertexShader(g_VertexShader, sizeof(g_VertexShader), nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	// Describe the memory layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT number_elements = ARRAYSIZE(layout);
	DX::Check(m_RenderDevice->GetDevice()->CreateInputLayout(layout, number_elements, g_VertexShader, sizeof(g_VertexShader), m_VertexLayout.ReleaseAndGetAddressOf()));
}

void Render::RenderShader::LoadPixelShader()
{
	m_RenderDevice->GetDevice()->CreatePixelShader(g_PixelShader, sizeof(g_PixelShader), nullptr, m_PixelShader.ReleaseAndGetAddressOf());
}

void Render::RenderShader::UpdateSize(int width, int height, bool stretch)
{
	Render::CameraBuffer buffer = {};

	XMMATRIX world = XMMatrixIdentity();

	if (stretch)
	{
		world *= XMMatrixScaling(width * 0.5f, height * 0.5f, 1.0f);
	}
	else
	{
		int texture_width = GameBoy::ScreenWidth;
		int texture_height = GameBoy::ScreenHeight;

		texture_width = static_cast<int>(width);
		texture_height = static_cast<int>(width * 0.9f);

		if (texture_height > height)
		{
			texture_width = static_cast<int>(height * 1.11f);
			texture_height = static_cast<int>(height);
		}

		world *= XMMatrixScaling(texture_width * 0.5f, texture_height * 0.5f, 1.0f);
	}

	buffer.world = XMMatrixTranspose(world);

	buffer.camera_projection = XMMatrixOrthographicLH((float)width, (float)height, 0.0f, 1.0f);

	m_RenderDevice->GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &buffer, 0, 0);
}