#pragma once

#include "Renderer.h"
#include <string>

namespace DX
{
	class Shader
	{
	public:
		Shader(Renderer* renderer);
		virtual ~Shader() = default;

		// Create vertex shader
		void LoadVertexShader(std::wstring&& vertex_shader_path);

		// Create pixel shader
		void LoadPixelShader(std::wstring&& pixel_shader_path);

		// Bind the shader to the pipeline
		void Use();

	private:
		Renderer* m_DxRenderer = nullptr;

		// Vertex shader
		ComPtr<ID3D11VertexShader> m_d3dVertexShader = nullptr;

		// Vertex shader input layout
		ComPtr<ID3D11InputLayout> m_d3dVertexLayout = nullptr;

		// Pixel shader
		ComPtr<ID3D11PixelShader> m_d3dPixelShader = nullptr;

		// Texture filter
		ComPtr<ID3D11SamplerState> m_AnisotropicSampler = nullptr;
	};
}