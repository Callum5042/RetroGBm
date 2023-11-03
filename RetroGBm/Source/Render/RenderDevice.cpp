#include "Render/RenderDevice.h"
#include <exception>
#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"

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

void Render::RenderDevice::Create()
{
	CreateDeviceAndContext();
}

void Render::RenderDevice::CreateDeviceAndContext()
{
	// Look for Direct3D 11 feature
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// Add debug flag if in debug mode
	D3D11_CREATE_DEVICE_FLAG deviceFlag = (D3D11_CREATE_DEVICE_FLAG)0;
#ifdef _DEBUG
	deviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create device and device context
	D3D_FEATURE_LEVEL featureLevel;
	DX::Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, m_Device.ReleaseAndGetAddressOf(), &featureLevel, m_DeviceContext.ReleaseAndGetAddressOf()));

	// Check if Direct3D 11.1 is supported, if not fall back to Direct3D 11
	if (featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		MessageBox(NULL, L"D3D_FEATURE_LEVEL_11_1 is not supported! Falling back to D3D_FEATURE_LEVEL_11_0", L"Error", MB_OK);
	}

	// Check if Direct3D 11 is supported
	if (featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		MessageBox(NULL, L"D3D_FEATURE_LEVEL_11_0 is not supported", L"Error", MB_OK);
		throw std::exception();
	}
}

std::unique_ptr<Render::RenderTarget> Render::RenderDevice::CreateRenderTarget()
{
	return std::make_unique<Render::RenderTarget>(this);
}

std::unique_ptr<Render::RenderTexture> Render::RenderDevice::CreateTexture()
{
	return std::make_unique<Render::RenderTexture>(this);
}