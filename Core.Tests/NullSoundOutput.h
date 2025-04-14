#pragma once

#include "RetroGBm/Audio/ISoundOutput.h"

class NullSoundOutput : public ISoundOutput
{
public:
    NullSoundOutput() = default;
    virtual ~NullSoundOutput() = default;

    void Start();
    void Stop();
    void Play(int left, int right);
};