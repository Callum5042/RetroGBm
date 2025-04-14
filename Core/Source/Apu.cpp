#include "RetroGBm/Pch.h"
#include "RetroGBm/Apu.h"
#include "RetroGBm/Audio/SoundMode1.h"
#include "RetroGBm/Audio/SoundMode2.h"
#include "RetroGBm/Audio/SoundMode3.h"
#include "RetroGBm/Audio/SoundMode4.h"

Apu::Apu(ISoundOutput* output) : m_SoundOutput(output)
{
	m_SoundChannels[0] = std::make_unique<SoundMode1>(&Context);
	m_SoundChannels[1] = std::make_unique<SoundMode2>(&Context);
	m_SoundChannels[2] = std::make_unique<SoundMode3>(&Context);
	m_SoundChannels[3] = std::make_unique<SoundMode4>(&Context);
}

void Apu::Tick()
{
	if (!m_Enabled)
	{
		return;
	}

	// Tick each channel
	int left = 0, right = 0;
	for (int i = 0; i < 4; i++)
	{
		int channel_sample = m_SoundChannels[i]->TickChannel();

		// Sound panning
		int selection = Context.sound_panning;
		if ((selection & (1 << (i + 4))) != 0)
		{
			left += channel_sample;
		}

		if ((selection & (1 << i)) != 0)
		{
			right += channel_sample;
		}
	}

	// Volume
	int volumes = Context.master_volume;
	left *= (volumes >> 4) & 0b111;
	right *= volumes & 0b111;

	m_SoundOutput->Play(left / 4, right / 4);
}

void Apu::Write(uint16_t address, uint8_t value)
{
	if (address == 0xFF26)
	{
		Context.audio_master = value;

		bool audio_on = (value & 0x80) == 0x80;
		if (audio_on)
		{
			if (!m_Enabled)
			{
				m_Enabled = true;
				Start();
			}
		}
		else
		{
			if (m_Enabled)
			{
				m_Enabled = false;
				Stop();
			}
		}

		return;
	}
	else if (address == 0xFF25)
	{
		Context.sound_panning = value;
		return;
	}
	else if (address == 0xFF24)
	{
		Context.master_volume = value;
		return;
	}

	// Channel 1
	if (address >= 0xFF10 && address <= 0xFF14)
	{
		m_SoundChannels[0]->Write(address, value);
		return;
	}

	// Channel 2
	if (address >= 0xFF16 && address <= 0xFF19)
	{
		m_SoundChannels[1]->Write(address, value);
		return;
	}

	// Channel 3
	if ((address >= 0xFF1A && address <= 0xFF1E) || (address >= 0xFF30 && address <= 0xFF3F))
	{
		m_SoundChannels[2]->Write(address, value);
		return;
	}

	// Channel 4
	if (address >= 0xFF20 && address <= 0xFF23)
	{
		m_SoundChannels[3]->Write(address, value);
		return;
	}
}

int Apu::Read(uint16_t address)
{
	if (address == 0xFF26)
	{
		int result = 0;
		for (int i = 0; i < 4; i++)
		{
			result |= m_SoundChannels[i]->IsEnabled() ? (1 << i) : 0;
		}

		result |= m_Enabled ? (1 << 7) : 0;
		return result | 0x70;
	}
	else if (address == 0xFF25)
	{
		return Context.sound_panning;
	}
	else if (address == 0xFF24)
	{
		return Context.master_volume;
	}

	// Channel 1
	if (address >= 0xFF10 && address <= 0xFF14)
	{
		return m_SoundChannels[0]->Read(address);
	}

	// Channel 2
	if (address >= 0xFF16 && address <= 0xFF19)
	{
		return m_SoundChannels[1]->Read(address);
	}

	// Channel 3
	if ((address >= 0xFF1A && address <= 0xFF1E) || (address >= 0xFF30 && address <= 0xFF3F))
	{
		return m_SoundChannels[2]->Read(address);
	}

	// Channel 4
	if (address >= 0xFF20 && address <= 0xFF23)
	{
		return m_SoundChannels[3]->Read(address);
	}

	return 0xFF;
}

void Apu::Start()
{
	for (int address = 0xFF10; address <= 0xFF25; address++)
	{
		int value = 0;

		// lengths should be preserved
		if (address == 0xFF11)
		{
			value = Context.channel1_nr11 & 0x3F;
		}
		else if (address == 0xFF16)
		{
			value = Context.channel2_nr21 & 0x3F;
		}
		else if (address == 0xFF1B)
		{
			value = Context.channel3_nr31;
		}
		else if (address == 0xFF20)
		{
			value = Context.channel4_nr41 & 0x3F;
		}

		this->Write(address, value);
	}

	for (auto& channel : m_SoundChannels)
	{
		channel->Start();
	}

	m_SoundOutput->Start();
}

void Apu::Stop()
{
	m_SoundOutput->Stop();
	for (auto& channel : m_SoundChannels)
	{
		channel->Stop();
	}
}
