#include "Render/DisplayOutput.h"

DisplayOutput::DisplayOutput(Render::RenderTexture* render_texture) : m_RenderTexture(render_texture)
{
}

void DisplayOutput::Draw(void* pixel_buffer, int video_pitch)
{
	PixelBuffer = pixel_buffer;

	// m_RenderTexture->Update(pixel_buffer, video_pitch);
}