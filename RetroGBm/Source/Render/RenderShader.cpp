#include "Render/RenderShader.h"
#include "Render/RenderDevice.h"
#include <D3DCompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

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
	LoadPixelShader(L"D:/Sources/RetroGBm/RetroGBm/Shaders/PixelShader.hlsl");
	LoadVertexShader(L"D:/Sources/RetroGBm/RetroGBm/Shaders/VertexShader.hlsl");

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

void Render::RenderShader::LoadVertexShader(const std::wstring& vertex_shader_path)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

	ComPtr<ID3DBlob> shader_blob = nullptr;
	ID3DBlob* error = nullptr;
	DX::Check(D3DCompileFromFile(vertex_shader_path.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", flags, 0, shader_blob.GetAddressOf(), &error));

	// Create the vertex shader
	DX::Check(m_RenderDevice->GetDevice()->CreateVertexShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	// Describe the memory layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	DX::Check(m_RenderDevice->GetDevice()->CreateInputLayout(layout, numElements, shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), m_VertexLayout.ReleaseAndGetAddressOf()));
}

void Render::RenderShader::LoadPixelShader(const std::wstring& pixel_shader_path)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

	ComPtr<ID3DBlob> shader_blob = nullptr;
	ID3DBlob* error = nullptr;
	DX::Check(D3DCompileFromFile(pixel_shader_path.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", flags, 0, shader_blob.GetAddressOf(), &error));

	// Create pixel shader
	DX::Check(m_RenderDevice->GetDevice()->CreatePixelShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, m_PixelShader.ReleaseAndGetAddressOf()));
}