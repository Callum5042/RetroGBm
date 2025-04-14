#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/FrequencySweep.h"
#include "RetroGBm/Constants.h"

FrequencySweep::FrequencySweep()
{
    m_Divider = GameBoy::TicksPerSec / 128;
}

void FrequencySweep::Start()
{
    m_CounterEnabled = false;
    m_Tick = 8192;
}

void FrequencySweep::Trigger()
{
    m_Negging = false;
    m_Overflow = false;

    m_ShadowFrequenecy = m_Nr13 | ((m_Nr14 & 0b111) << 8);
    m_Timer = m_SweepPace == 0 ? 8 : m_SweepPace;
    m_CounterEnabled = m_SweepPace != 0 || m_SweepStep != 0;

    if (m_SweepStep > 0)
    {
        Calculate();
    }
}

void FrequencySweep::SetNr10(int value)
{
    m_SweepPace = (value >> 4) & 0b111;
    m_DirectionSubtract = (value & (1 << 3)) != 0;
    m_SweepStep = (value & 0b111);

    if (m_Negging && !m_DirectionSubtract)
    {
        m_Overflow = true;
    }
}

void FrequencySweep::SetNr13(int value)
{
    m_Nr13 = value;
}

void FrequencySweep::SetNr14(int value)
{
    m_Nr14 = value;
    if ((value & (1 << 7)) != 0)
    {
        Trigger();
    }
}

void FrequencySweep::Tick()
{
    // Only perform frequency sweep every 32,768 ticks (128Hz)
    m_Tick++;

    if (m_Tick != m_Divider)
        return;

    m_Tick = 0;

    if (!m_CounterEnabled)
        return;

    // Sweeps
    m_Timer--;

    if (m_Timer != 0)
        return;

    m_Timer = m_SweepPace == 0 ? 8 : m_SweepPace;

    if (m_SweepPace == 0)
        return;

    // Calculate frequency
    int new_frequency = Calculate();

    if (m_Overflow || m_SweepStep == 0)
        return;

    m_ShadowFrequenecy = new_frequency;
    m_Nr13 = m_ShadowFrequenecy & 0xFF;
    m_Nr14 = (m_ShadowFrequenecy & 0x700) >> 8;

    Calculate();
}

int FrequencySweep::Calculate()
{
    int frequency = m_ShadowFrequenecy >> m_SweepStep;
    if (m_DirectionSubtract)
    {
        frequency = m_ShadowFrequenecy - frequency;
        m_Negging = true;
    }
    else
    {
        frequency = m_ShadowFrequenecy + frequency;
    }

    // Pandocs: When it overflows (being clocked when it’s already 2047, or $7FF), its value is set from the contents of NR13 and NR14
    if (frequency > 2047)
    {
        m_Overflow = true;
    }

    return frequency;
}