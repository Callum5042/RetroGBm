#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/VolumeEnvelope.h"
#include "RetroGBm/Constants.h"

VolumeEnvelope::VolumeEnvelope()
{
}

void VolumeEnvelope::SetRegister(int reg)
{
    m_InitialVolume = (reg >> 4);
    m_EnvelopeDirection = (reg & (1 << 3)) == 0 ? -1 : 1;
    m_Sweep = reg & 0x7;
}

bool VolumeEnvelope::IsEnabled()
{
    return m_Sweep > 0;
}

void VolumeEnvelope::Start()
{
    m_Finished = true;
    m_Tick = 8192;
}

void VolumeEnvelope::Trigger()
{
    m_Tick = 0;
    m_Volume = m_InitialVolume;
    m_Finished = false;
}

void VolumeEnvelope::Tick()
{
    if (m_Finished)
    {
        return;
    }

    if ((m_Volume == 0 && m_EnvelopeDirection == -1) || (m_Volume == 15 && m_EnvelopeDirection == 1))
    {
        m_Finished = true;
        return;
    }

    m_Tick++;
    if (m_Tick == (m_Sweep * GameBoy::TicksPerSec / 64))
    {
        m_Tick = 0;
        m_Volume += m_EnvelopeDirection;
    }
}

int VolumeEnvelope::GetVolume()
{
    return IsEnabled() ? m_Volume : m_InitialVolume;
}