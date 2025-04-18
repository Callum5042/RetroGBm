#pragma once

class IDisplayOutput
{
public:
	IDisplayOutput() = default;
	virtual ~IDisplayOutput() = default;

	virtual void Draw(void* pixel_buffer) = 0;
};