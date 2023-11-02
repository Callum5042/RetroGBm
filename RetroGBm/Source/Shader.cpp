#include "Shader.h"
#include <fstream>
#include <vector>
#include <D3DCompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

DX::Shader::Shader(Renderer* renderer) : m_DxRenderer(renderer)
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 1000.0f;

	DX::Check(m_DxRenderer->GetDevice()->CreateSamplerState(&samplerDesc, &m_AnisotropicSampler));
}

void DX::Shader::LoadVertexShader(std::wstring&& vertex_shader_path)
{
	auto d3dDevice = m_DxRenderer->GetDevice();

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

	ComPtr<ID3DBlob> shader_blob = nullptr;
	ID3DBlob* error = nullptr;
	DX::Check(D3DCompileFromFile(vertex_shader_path.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", flags, 0, shader_blob.GetAddressOf(), &error));

	// Create the vertex shader
	DX::Check(d3dDevice->CreateVertexShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, m_d3dVertexShader.ReleaseAndGetAddressOf()));

	// Describe the memory layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	DX::Check(d3dDevice->CreateInputLayout(layout, numElements, shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), m_d3dVertexLayout.ReleaseAndGetAddressOf()));
}

void DX::Shader::LoadPixelShader(std::wstring&& pixel_shader_path)
{
	auto d3dDevice = m_DxRenderer->GetDevice();

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

	ComPtr<ID3DBlob> shader_blob = nullptr;
	ID3DBlob* error = nullptr;
	DX::Check(D3DCompileFromFile(pixel_shader_path.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", flags, 0, shader_blob.GetAddressOf(), &error));

	// Create pixel shader
	DX::Check(d3dDevice->CreatePixelShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, m_d3dPixelShader.ReleaseAndGetAddressOf()));
}

void DX::Shader::Use()
{
	auto d3dDeviceContext = m_DxRenderer->GetDeviceContext();

	// Bind the input layout to the pipeline's Input Assembler stage
	d3dDeviceContext->IASetInputLayout(m_d3dVertexLayout.Get());

	// Bind the vertex shader to the pipeline's Vertex Shader stage
	d3dDeviceContext->VSSetShader(m_d3dVertexShader.Get(), nullptr, 0);

	// Bind the pixel shader to the pipeline's Pixel Shader stage
	d3dDeviceContext->PSSetShader(m_d3dPixelShader.Get(), nullptr, 0);

	// Bind pixel shader texture sampler
	d3dDeviceContext->PSSetSamplers(0, 1, m_AnisotropicSampler.GetAddressOf());
}
