#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/SoundMode1.h"
#include <exception>

#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Apu.h"

SoundMode1::SoundMode1(SoundContext* context) : SoundModeBase(0xFF10, 64)
{
    m_Context = context;
}

void SoundMode1::Start()
{
    m_Tick = 0;

    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();
    if (Gbc)
    {
        Length->Reset();
    }

    Length->Start();
    m_FrequencySweep.Start();
    m_VolumeEnvelope.Start();
}

void SoundMode1::Trigger()
{
    m_Tick = 0;
    m_FrequencyDivider = 1;
    m_VolumeEnvelope.Trigger();
}

int SoundMode1::TickChannel()
{
    m_VolumeEnvelope.Tick();

    UpdateLength();
    UpdateSweep();

    if (ChannelEnabled || DacEnabled)
    {
        m_FrequencyDivider--;
        if (m_FrequencyDivider == 0)
        {
            ResetFrequencyDivider();
            m_LastOutput = ((GetDuty() & (1 << m_Tick)) >> m_Tick);
            m_Tick = (m_Tick + 1) % 8;
        }

        return m_LastOutput * m_VolumeEnvelope.GetVolume();
    }

    return 0;
}

void SoundMode1::SetNr0(int value)
{
    m_Context->channel1_nr10 = value;
    m_FrequencySweep.SetNr10(value);
}

void SoundMode1::SetNr1(int value)
{
    m_Context->channel1_nr11 = value;
    Length->SetLength(64 - (value & 0b00111111));
}

void SoundMode1::SetNr2(int value)
{
    m_Context->channel1_nr12 = value;

    m_VolumeEnvelope.SetRegister(value);
    DacEnabled = (value & 0b11111000) != 0;
    ChannelEnabled &= DacEnabled;
}

void SoundMode1::SetNr3(int value)
{
    m_Context->channel1_nr13 = value;
    m_FrequencySweep.SetNr13(value);
}

void SoundMode1::SetNr4(int value)
{
    m_Context->channel1_nr14 = value;

    Length->SetRegister(value);
    if ((value & (1 << 7)) != 0)
    {
        ChannelEnabled = DacEnabled;
        Trigger();
    }

    m_FrequencySweep.SetNr14(value);
}

int SoundMode1::GetNr0()
{
    return m_Context->channel1_nr10;
}

int SoundMode1::GetNr1()
{
    return m_Context->channel1_nr11;
}

int SoundMode1::GetNr2()
{
    return m_Context->channel1_nr12;
}

int SoundMode1::GetNr3()
{
    return m_FrequencySweep.GetNr13();
}

int SoundMode1::GetNr4()
{
    return (m_Context->channel1_nr14 & 0b11111000) | (m_FrequencySweep.GetNr14() & 0b00000111);
}

int SoundMode1::GetDuty()
{
    switch (GetNr1() >> 6)
    {
        case 0:
            return 0b00000001;
        case 1:
            return 0b10000001;
        case 2:
            return 0b10000111;
        case 3:
            return 0b01111110;
        default:
            throw std::exception("Illegal state exception");
    }
}

void SoundMode1::ResetFrequencyDivider()
{
    int channel_frequency = 2048 - (GetNr3() | ((GetNr4() & 0b111) << 8));
    m_FrequencyDivider = channel_frequency * 4;
}

void SoundMode1::UpdateSweep()
{
    m_FrequencySweep.Tick();
    if (ChannelEnabled && !m_FrequencySweep.IsEnabled())
    {
        ChannelEnabled = false;
    }
}

void SoundMode1::Write(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case 0xFF10:
            SetNr0(value);
            break;
        case 0xFF11:
            SetNr1(value);
            break;
        case 0xFF12:
            SetNr2(value);
            break;
        case 0xFF13:
            SetNr3(value);
            break;
        case 0xFF14:
            SetNr4(value);
            break;
    }
}

int SoundMode1::Read(uint16_t address)
{
    switch (address)
    {
        case 0xFF10:
            return GetNr0() | 0x80;
        case 0xFF11:
            return GetNr1() | 0x3F;
        case 0xFF12:
            return GetNr2() | 0x00;
        case 0xFF13:
            return GetNr3() | 0xFF;
        case 0xFF14:
            return GetNr4() | 0xBF;
        default:
            throw std::exception("Illegal state");
    }
}