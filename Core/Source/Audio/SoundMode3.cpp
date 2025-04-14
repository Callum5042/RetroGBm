#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/SoundMode3.h"

#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Apu.h"

SoundMode3::SoundMode3(SoundContext* context) : SoundModeBase(0xFF1A, 256)
{
    m_Context = context;

    m_WaveRam.resize(16, 0);

    int i = 0;

    // bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();
    if (true)
    { 
        for (auto& v : CgbWave)
        {
            m_WaveRam[i++] = v;
        }
    }
    else
    {
        for (auto& v : DmgWave)
        {
            m_WaveRam[i++] = v;
        }
    }
}

int SoundMode3::Read(uint16_t address)
{
    // Wave RAM
    if (address >= 0xFF30 && address <= 0xFF3F)
    {
        if (!IsEnabled())
        {
            return m_WaveRam[address - 0xFF30];
        }

        bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();

        if (Gbc || m_TicksSinceRead < 2)
        {
            return m_WaveRam[m_LastReadAddress];
        }

        return 0xff;
    }

    // Registers
    switch (address)
    {
        case 0xFF1A:
            return GetNr0() | 0x7F;
        case 0xFF1B:
            return GetNr1() | 0xFF;
        case 0xFF1C:
            return GetNr2() | 0x9F;
        case 0xFF1D:
            return GetNr3() | 0xFF;
        case 0xFF1E:
            return GetNr4() | 0xBF;
        default:
            throw std::exception("Illegal state");
    }
}

void SoundMode3::Write(uint16_t address, uint8_t value)
{
    // Wave RAM
    if (address >= 0xFF30 && address <= 0xFF3F)
    {
        bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();

        if (!IsEnabled())
        {
            m_WaveRam[address - 0xFF30] = value;
        }
        else if (Gbc || m_TicksSinceRead < 2)
        {
            m_WaveRam[m_LastReadAddress] = value;
        }
    }

    // Registers
    switch (address)
    {
        case 0xFF1A:
            SetNr0(value);
            break;
        case 0xFF1B:
            SetNr1(value);
            break;
        case 0xFF1C:
            SetNr2(value);
            break;
        case 0xFF1D:
            SetNr3(value);
            break;
        case 0xFF1E:
            SetNr4(value);
            break;
    }
}

void SoundMode3::SetNr0(int value)
{
    m_Context->channel3_nr30 = value;

    DacEnabled = (value & (1 << 7)) != 0;
    ChannelEnabled &= DacEnabled;
}

void SoundMode3::SetNr1(int value)
{
    m_Context->channel3_nr31 = value;

    Length->SetLength(256 - value);
}

void SoundMode3::SetNr2(int value)
{
    m_Context->channel3_nr32 = value;
}

void SoundMode3::SetNr3(int value)
{
    m_Context->channel3_nr33 = value;
}

void SoundMode3::SetNr4(int value)
{
    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();
    if (!Gbc && (value & (1 << 7)) != 0)
    {
        if (IsEnabled() && m_FrequencyDivider == 2)
        {
            auto pos = m_Tick / 2;
            if (pos < 4)
            {
                m_WaveRam[0] = m_WaveRam[pos];
            }
            else
            {
                pos &= ~3;
                for (int j = 0; j < 4; j++)
                {
                    uint8_t data = m_WaveRam[(pos + j) % 0x10];
                    m_WaveRam[j] = data;
                }
            }
        }
    }

    m_Context->channel3_nr34 = value;

    Length->SetRegister(value);
    if ((value & (1 << 7)) != 0)
    {
        ChannelEnabled = DacEnabled;
        Trigger();
    }
}

int SoundMode3::GetNr0()
{
    return m_Context->channel3_nr30;
}

int SoundMode3::GetNr1()
{
    return m_Context->channel3_nr31;
}

int SoundMode3::GetNr2()
{
    return m_Context->channel3_nr32;
}

int SoundMode3::GetNr3()
{
    return m_Context->channel3_nr33;
}

int SoundMode3::GetNr4()
{
    return m_Context->channel3_nr34;
}

void SoundMode3::Start()
{
    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();

    m_Tick = 0;
    m_Buffer = 0;
    if (Gbc)
    {
        Length->Reset();
    }

    Length->Start();
}

void SoundMode3::Trigger()
{
    m_Tick = 0;
    m_FrequencyDivider = 6;
    m_Triggered = !true;

    bool Gbc = !Emulator::Instance->GetCartridge()->IsColourModeDMG();
    if (Gbc)
    {
        GetWaveEntry();
    }
}

int SoundMode3::TickChannel()
{
    m_TicksSinceRead++;
    UpdateLength();

    if (!ChannelEnabled)
    {
        return 0;
    }

    if (!DacEnabled)
    {
        return 0;
    }

    if ((GetNr0() & (1 << 7)) == 0)
    {
        return 0;
    }

    m_FrequencyDivider--;
    if (m_FrequencyDivider == 0)
    {
        ResetFreqDivider();
        if (m_Triggered)
        {
            m_LastOutput = (m_Buffer >> 4) & 0x0F;
            m_Triggered = false;
        }
        else
        {
            m_LastOutput = GetWaveEntry();
        }

        m_Tick = (m_Tick + 1) % 32;
    }

    return m_LastOutput;
}

int SoundMode3::GetVolume()
{
    return (GetNr2() >> 5) & 0b11;
}

int SoundMode3::GetWaveEntry()
{
    m_TicksSinceRead = 0;
    m_LastReadAddress = m_Tick / 2;
    m_Buffer = m_WaveRam[m_LastReadAddress];

    int b = m_Buffer;
    if (m_Tick % 2 == 0)
    {
        b = (b >> 4) & 0x0F;
    }
    else
    {
        b &= 0x0F;
    }

    switch (GetVolume())
    {
		case 0:
			return 0;
		case 1:
            return b;
		case 2:
            return b >> 1;
		case 3:
            return b >> 2;
		default:
			throw std::exception("Illegal state");
    }
}

void SoundMode3::ResetFreqDivider()
{
    int channel_frequency = 2048 - (GetNr3() | ((GetNr4() & 0b111) << 8));
    m_FrequencyDivider = channel_frequency * 2;
}