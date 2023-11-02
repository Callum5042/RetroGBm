#pragma once

#include "Renderer.h"
#include <vector>

namespace DX
{
	struct Vertex
	{
		// Vertex position
		float x = 0;
		float y = 0;
		float z = 0;

		// Texture UV
		float u = 0;
		float v = 0;
	};

	class Model
	{
	public:
		Model(DX::Renderer* renderer);
		virtual ~Model() = default;

		// Create device
		void Create();

		// Render the model
		void Render();

		// Update texture
		void UpdateTexture(void* video_buffer, int video_pitch);

	private:
		DX::Renderer* m_DxRenderer = nullptr;

		// Number of vertices to draw
		UINT m_VertexCount = 0;
		UINT m_IndexCount = 0;

		// Vertex buffer
		ComPtr<ID3D11Buffer> m_d3dVertexBuffer = nullptr;

		// Index buffer
		ComPtr<ID3D11Buffer> m_d3dIndexBuffer = nullptr;

		// Texture resource
		ComPtr<ID3D11Texture2D> m_Texture = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_TexturShadereResourceView = nullptr;
		void CreateTexture();
	};
}