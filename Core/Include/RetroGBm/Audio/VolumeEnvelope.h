#pragma once

class VolumeEnvelope
{
public:
	VolumeEnvelope();

    void SetRegister(int reg);
    bool IsEnabled();
    void Start();
    void Trigger();

    void Tick();
    int GetVolume();

private:
    int m_Tick = 0;

    int m_InitialVolume = 0;
    int m_EnvelopeDirection = 0;
    int m_Sweep = 0;
    int m_Volume = 0;
    bool m_Finished = false;
};