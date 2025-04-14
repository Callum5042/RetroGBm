#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/SoundMode4.h"

#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Apu.h"
#include <exception>

SoundMode4::SoundMode4(SoundContext* context) : SoundModeBase(0xFF1F, 64)
{
    m_Context = context;
}

void SoundMode4::Start()
{
    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();

    if (Gbc)
    {
        Length->Reset();
    }

    Length->Start();
    m_Lfsr.Start();
    m_VolumeEnvelope.Start();
}

void SoundMode4::Trigger()
{
    m_Lfsr.Reset();
    m_VolumeEnvelope.Trigger();
}

int SoundMode4::TickChannel()
{
    m_VolumeEnvelope.Tick();
    UpdateLength();

    if (!ChannelEnabled)
    {
        return 0;
    }

    if (!DacEnabled)
    {
        return 0;
    }

    if (m_PolynomialCounter.Tick())
    {
        m_LastResult = m_Lfsr.NextBit(((m_Context->channel4_nr43 >> 3) & 1) != 0);
    }

    return m_LastResult * m_VolumeEnvelope.GetVolume();
}

void SoundMode4::SetNr1(int value)
{
    m_Context->channel4_nr41 = value;

    Length->SetLength(64 - (value & 0b00111111));
}

void SoundMode4::SetNr2(int value)
{
    m_Context->channel4_nr42 = value;

    m_VolumeEnvelope.SetRegister(value);
    DacEnabled = (value & 0b11111000) != 0;
    ChannelEnabled &= DacEnabled;
}

void SoundMode4::SetNr3(int value)
{
    m_Context->channel4_nr43 = value;
    m_PolynomialCounter.SetRegister(value);
}

void SoundMode4::SetNr4(int value)
{
    m_Context->channel4_nr44 = value;

    Length->SetRegister(value);
    if ((value & (1 << 7)) != 0)
    {
        ChannelEnabled = DacEnabled;
        Trigger();
    }
}

int SoundMode4::GetNr1()
{
    return m_Context->channel4_nr41;
}

int SoundMode4::GetNr2()
{
    return m_Context->channel4_nr42;
}

int SoundMode4::GetNr3()
{
    return m_Context->channel4_nr43;
}

int SoundMode4::GetNr4()
{
    return m_Context->channel4_nr44;
}

void SoundMode4::Write(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case 0xFF20:
            SetNr1(value);
            break;
        case 0xFF21:
            SetNr2(value);
            break;
        case 0xFF22:
            SetNr3(value);
            break;
        case 0xFF23:
            SetNr4(value);
            break;
    }
}

int SoundMode4::Read(uint16_t address)
{
    switch (address)
    {
        case 0xFF20:
            return GetNr1() | 0xFF;
        case 0xFF21:
            return GetNr2() | 0x00;
        case 0xFF22:
            return GetNr3() | 0x00;
        case 0xFF23:
            return GetNr4() | 0xBF;
        default:
            throw std::runtime_error("Illegal state");
    }
}