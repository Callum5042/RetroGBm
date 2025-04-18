#pragma once

#include "RetroGBm/IDisplayOutput.h"

class NullDisplayOutput : public IDisplayOutput
{
public:
    NullDisplayOutput() = default;
    virtual ~NullDisplayOutput() = default;

    void Draw(void* pixel_buffer, int pixel_pitch) override;
};