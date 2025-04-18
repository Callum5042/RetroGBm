#pragma once

#include <RetroGBm/IDisplayOutput.h>
#include "Render/RenderTexture.h"

#include <vector>

class DisplayOutput : public IDisplayOutput
{
	Render::RenderTexture* m_RenderTexture = nullptr;

public:
	DisplayOutput(Render::RenderTexture* render_texture);
	virtual ~DisplayOutput() = default;

	void Draw(void* pixel_buffer, int video_pitch);

	void* PixelBuffer = nullptr;
};