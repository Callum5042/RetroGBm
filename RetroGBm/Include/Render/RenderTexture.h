#pragma once

#include <d3d11_1.h>

// This include is requires for using DirectX smart pointers (ComPtr)
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

namespace Render
{
	class RenderDevice;

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

	class RenderTexture
	{
		RenderDevice* m_RenderDevice = nullptr;

	public:
		RenderTexture(RenderDevice* device);
		virtual ~RenderTexture() = default;

		// Create device
		void Create(int width, int height);

		// Render the model
		void Render();

		// Update texture
		void Update(void* video_buffer, int video_pitch);

	private:
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

		// Size
		int m_Width = 0;
		int m_Height = 0;
	};
}