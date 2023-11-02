#include "Model.h"
#include <vector>
#include <DirectXMath.h>
#include <DirectXColors.h>

DX::Model::Model(DX::Renderer* renderer) : m_DxRenderer(renderer)
{
}

void DX::Model::Create()
{
	auto d3dDevice = m_DxRenderer->GetDevice();

	// Vertex data
	std::vector<Vertex> vertices =
	{
		{ -1.0f, +1.0f, 0.0f, -0.0f, -0.0f }, // Top left vertex
		{ +1.0f, +1.0f, 0.0f, +1.0f, -0.0f }, // Top right vertex
		{ -1.0f, -1.0f, 0.0f, -0.0f, +1.0f }, // Bottom left vertex
		{ +1.0f, -1.0f, 0.0f, +1.0f, +1.0f }, // Bottom right vertex
	};

	m_VertexCount = static_cast<UINT>(vertices.size());

	// Index data
	std::vector<UINT> indices =
	{
		0, 1, 2,
		2, 1, 3,
	};

	m_IndexCount = static_cast<UINT>(indices.size());

	// Create vertex buffer
	D3D11_BUFFER_DESC vertexbuffer_desc = {};
	vertexbuffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertexbuffer_desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
	vertexbuffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertex_subdata = {};
	vertex_subdata.pSysMem = vertices.data();

	DX::Check(d3dDevice->CreateBuffer(&vertexbuffer_desc, &vertex_subdata, m_d3dVertexBuffer.ReleaseAndGetAddressOf()));

	// Create index buffer
	D3D11_BUFFER_DESC index_buffer_desc = {};
	index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	index_buffer_desc.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA index_subdata = {};
	index_subdata.pSysMem = indices.data();

	DX::Check(d3dDevice->CreateBuffer(&index_buffer_desc, &index_subdata, m_d3dIndexBuffer.ReleaseAndGetAddressOf()));

	// Create texture
	CreateTexture();
}

void DX::Model::Render()
{
	auto d3dDeviceContext = m_DxRenderer->GetDeviceContext();

	// We need the stride and offset
	UINT vertex_stride = sizeof(Vertex);
	auto vertex_offset = 0u;

	// Bind the vertex buffer to the Input Assembler
	d3dDeviceContext->IASetVertexBuffers(0, 1, m_d3dVertexBuffer.GetAddressOf(), &vertex_stride, &vertex_offset);

	// Bind the index buffer to the Input Assembler
	d3dDeviceContext->IASetIndexBuffer(m_d3dIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Bind the geometry topology to the Input Assembler
	d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind texture to the pixel shader
	d3dDeviceContext->PSSetShaderResources(0, 1, m_TexturShadereResourceView.GetAddressOf());

	// Render geometry
	d3dDeviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void DX::Model::UpdateTexture(void* video_buffer, int video_pitch)
{
	auto deviceContext = m_DxRenderer->GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE resource = {};

	//  Disable GPU access to the vertex buffer data
	deviceContext->Map(m_Texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	// Data
	uint8_t* src = static_cast<uint8_t*>(video_buffer);
	uint8_t* dst = static_cast<uint8_t*>(resource.pData);

	// Update the texture
	const int HEIGHT = 144;
	for (int row = 0; row < HEIGHT; ++row)
	{
		std::memcpy(dst, src, resource.RowPitch);
		src += video_pitch;
		dst += resource.RowPitch;
	}

	//  Enable GPU access to the vertex buffer data
	deviceContext->Unmap(m_Texture.Get(), 0);
}

void DX::Model::CreateTexture()
{
	auto d3dDevice = m_DxRenderer->GetDevice();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = 160;
	desc.Height = 144;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	DX::Check(d3dDevice->CreateTexture2D(&desc, nullptr, m_Texture.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;
	DX::Check(d3dDevice->CreateShaderResourceView(m_Texture.Get(), &SRVDesc, m_TexturShadereResourceView.GetAddressOf()));
}
