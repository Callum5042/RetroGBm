#pragma once

class ISoundOutput
{
public:
	ISoundOutput() = default;
    virtual ~ISoundOutput() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Play(int left, int right) = 0;
};