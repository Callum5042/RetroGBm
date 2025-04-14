#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/Lfsr.h"

Lfsr::Lfsr()
{
	Reset();
}

void Lfsr::Start()
{
	Reset();
}

void Lfsr::Reset()
{
    m_Value = 0x7FFF;
}

int Lfsr::NextBit(bool width_7bit)
{
    int _xor = ((m_Value & 1) ^ ((m_Value & 2) >> 1));
    m_Value >>= 1;
    m_Value |= _xor << 14;

    if (width_7bit)
    {
        m_Value |= (_xor << 6);
        m_Value &= 0x7F;
    }

    return 1 & ~m_Value;
}