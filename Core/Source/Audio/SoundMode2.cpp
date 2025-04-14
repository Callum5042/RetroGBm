#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/SoundMode2.h"

#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Apu.h"

SoundMode2::SoundMode2(SoundContext* context) : SoundModeBase(0xFF15, 64)
{
    m_Context = context;
}

void SoundMode2::Start()
{
    m_Tick = 0;

    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();
    if (Gbc)
    {
        Length->Reset();
    }

    Length->Start();
    m_VolumeEnvelope.Start();
}

void SoundMode2::Trigger()
{
    m_Tick = 0;
    m_FrequencyDivider = 1;
    m_VolumeEnvelope.Trigger();
}

int SoundMode2::TickChannel()
{
    m_VolumeEnvelope.Tick();
    UpdateLength();

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

void SoundMode2::SetNr1(int value)
{
    m_Context->channel2_nr21 = value;

    Length->SetLength(64 - (value & 0b00111111));
}

void SoundMode2::SetNr2(int value)
{
    m_Context->channel2_nr22 = value;

    m_VolumeEnvelope.SetRegister(value);
    DacEnabled = (value & 0b11111000) != 0;
    ChannelEnabled &= DacEnabled;
}

void SoundMode2::SetNr3(int value)
{
    m_Context->channel2_nr23 = value;
}

void SoundMode2::SetNr4(int value)
{
    m_Context->channel2_nr24 = value;

    Length->SetRegister(value);
    if ((value & (1 << 7)) != 0)
    {
        ChannelEnabled = DacEnabled;
        Trigger();
    }
}

int SoundMode2::GetNr1()
{
    return m_Context->channel2_nr21;
}

int SoundMode2::GetNr2()
{
    return m_Context->channel2_nr22;
}

int SoundMode2::GetNr3()
{
    return m_Context->channel2_nr23;
}

int SoundMode2::GetNr4()
{
    return m_Context->channel2_nr24;
}

int SoundMode2::GetDuty()
{
    int i = (GetNr1() >> 6);

    switch (i)
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
			throw std::invalid_argument("Illegal operation");
    }
}

void SoundMode2::ResetFrequencyDivider()
{
    int channel_frequency = 2048 - (GetNr3() | ((GetNr4() & 0b111) << 8));
    m_FrequencyDivider = channel_frequency * 4;
}

void SoundMode2::Write(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case 0xFF16:
            SetNr1(value);
            break;
        case 0xFF17:
            SetNr2(value);
            break;
        case 0xFF18:
            SetNr3(value);
            break;
        case 0xFF19:
            SetNr4(value);
            break;
    }
}

int SoundMode2::Read(uint16_t address)
{
    switch (address)
    {
        case 0xFF16:
            return GetNr1() | 0x3F;
        case 0xFF17:
            return GetNr2() | 0x00;
        case 0xFF18:
            return GetNr3() | 0xFF;
        case 0xFF19:
            return GetNr4() | 0xBF;
        default:
            throw std::exception("Illegal state");
    }
}
