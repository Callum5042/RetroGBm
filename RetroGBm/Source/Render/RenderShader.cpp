#include "Render/RenderShader.h"
#include "Render/RenderDevice.h"

#include "../Shaders/PixelShader.hlsl.h"
#include "../Shaders/VertexShader.hlsl.h"

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

	// Sampler
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

	DX::Check(m_RenderDevice->GetDevice()->CreateSamplerState(&sampler_desc, &m_AnisotropicSampler));
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
	m_RenderDevice->GetDeviceContext()->PSSetSamplers(0, 1, m_AnisotropicSampler.GetAddressOf());
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