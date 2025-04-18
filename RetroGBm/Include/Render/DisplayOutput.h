#pragma once

#include <RetroGBm/IDisplayOutput.h>

class DisplayOutput : public IDisplayOutput
{
public:
	DisplayOutput() = default;
	virtual ~DisplayOutput() = default;

	void Draw(void* pixel_buffer);

private:

};