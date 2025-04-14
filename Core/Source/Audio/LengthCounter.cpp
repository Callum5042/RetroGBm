#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/LengthCounter.h"

LengthCounter::LengthCounter(int full_length)
{
    m_FullLength = full_length;
}

void LengthCounter::Start()
{
    m_Tick = 8192;
}

void LengthCounter::Tick()
{
    m_Tick++;

	if (m_Tick == m_Divider)
	{
        m_Tick = 0;
		if (m_Enabled && m_Length > 0)
		{
            m_Length--;
		}
	}
}

void LengthCounter::SetLength(int length)
{
    if (length == 0)
    {
        m_Length = m_FullLength;
    }
    else
    {
        m_Length = length;
    }
}

void LengthCounter::SetRegister(int value)
{
    bool enable = (value & (1 << 6)) != 0;
    bool trigger = (value & (1 << 7)) != 0;

    if (m_Enabled)
    {
        if (m_Length == 0 && trigger)
        {
            if (enable && m_Tick < m_Divider / 2)
            {
                SetLength(m_FullLength - 1);
            }
            else
            {
                SetLength(m_FullLength);
            }
        }
    }
    else if (enable)
    {
        if (m_Length > 0 && m_Tick < m_Divider / 2)
        {
            m_Length--;
        }

        if (m_Length == 0 && trigger && m_Tick < m_Divider / 2)
        {
            SetLength(m_FullLength - 1);
        }
    }
    else
    {
        if (m_Length == 0 && trigger)
        {
            SetLength(m_FullLength);
        }
    }

    m_Enabled = enable;
}

void LengthCounter::Reset()
{
    m_Enabled = true;
    m_Length = 0;
    m_Tick = 0;
}