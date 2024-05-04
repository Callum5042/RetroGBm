#include "RetroGBm/Pch.h"
#include "RetroGBm/Apu.h"
#include "RetroGBm/Timer.h"

#include <cstdint>
#include <iostream>

Apu::Apu(Timer* timer) : m_Timer(timer)
{
}

void Apu::Init()
{
	m_Context = {};

	m_WavePatternRam.resize(16);
	std::fill(m_WavePatternRam.begin(), m_WavePatternRam.end(), 0);
}

void Apu::Tick(bool doublespeed)
{
	IncrementApuTimer(doublespeed);
}

void Apu::IncrementApuTimer(bool doublespeed)
{
	// Bit 5 in doublespeed mode, otherwise bit 4
	uint8_t bitmask = doublespeed ? 0x32 : 0x16;

	bool old_apu_bitset = m_ApuBitset;
	m_ApuBitset = ((m_Timer->GetContext()->div & bitmask) == bitmask);

	// Increment APU timer if the bit has gone from a 1 to a 0
	if (old_apu_bitset && !m_ApuBitset)
	{
		m_ApuDiv++;

		// Channel 1
		if ((m_ApuDiv % 2) == 0)
		{
			// Channel 1
			if (m_LengthCounter1 != 0)
			{
				m_LengthCounter1--;
				if (m_LengthCounter1 == 0)
				{
					m_Context.audio_master &= ~0x1;
				}
			}

			// Channel 2
			if (m_LengthCounter2 != 0)
			{
				m_LengthCounter2--;
				if (m_LengthCounter2 == 0)
				{
					m_Context.audio_master &= ~0x2;
				}
			}

			// Channel 3
			if (m_LengthCounter3 != 0)
			{
				m_LengthCounter3--;
				if (m_LengthCounter3 == 0)
				{
					m_Context.audio_master &= ~0x4;
				}
			}

			// Channel 4
			if (m_LengthCounter4 != 0)
			{
				m_LengthCounter4--;
				if (m_LengthCounter4 == 0)
				{
					m_Context.audio_master &= ~0x8;
				}
			}
		}
	}
}

void Apu::Write(uint16_t address, uint8_t value)
{
	// std::cout << "Write APU: 0x" << std::hex << address << '\n';

	// Ignores writes to register if APU is off unless its the master control register 'NR52'
	if (!this->IsAudioOn() && address != 0xFF26)
	{
		return;
	}

	if (address == 0xFF26)
	{
		m_Context.audio_master = value & 0x80;
		if (!this->IsAudioOn())
		{
			m_Context.audio_master &= ~0xF;
			m_Context.sound_panning = 0x0;
			m_Context.master_volume = 0x0;

			// Channel 1
			m_Context.channel1_sweep = 0x0;
			m_Context.channel1_length = 0x0;
			m_Context.channel1_volume = 0x0;
			m_Context.channel1_periodlow = 0x0;
			m_Context.channel1_periodhigh = 0x0;

			// Channel 2
			m_Context.channel2_length = 0x0;
			m_Context.channel2_volume = 0;
			m_Context.channel2_periodlow = 0x0;
			m_Context.channel2_periodhigh = 0x0;

			// Channel 3
			m_Context.channel3_dac_enable = 0x0;
			m_Context.channel3_length = 0x0;
			m_Context.channel3_output_level = 0x0;
			m_Context.channel3_perioidlow = 0x0;
			m_Context.channel3_perioidhigh = 0x0;

			// Channel 4
			m_Context.channel4_length = 0x0;
			m_Context.channel4_volume = 0x0;
			m_Context.channel4_frequency = 0x0;
			m_Context.channel4_control = 0x0;

			// PCM
			m_Context.pcm12 = 0;
			m_Context.pcm34 = 0;
		}
	}
	else if (address == 0xFF25)
	{
		m_Context.sound_panning = value;
	}
	else if (address == 0xFF24)
	{
		m_Context.master_volume = value;
	}

	// Channel 1
	if (address == 0xFF10)
	{
		m_Context.channel1_sweep = value;
	}
	else if (address == 0xFF11)
	{
		m_Context.channel1_length = value;
		m_LengthCounter1 = 63 - (value & 0x3F);
	}
	else if (address == 0xFF12)
	{
		m_Context.channel1_volume = value;
	}
	else if (address == 0xFF13)
	{
		m_Context.channel1_periodlow = value;
	}
	else if (address == 0xFF14)
	{
		m_Context.channel1_periodhigh = value;

		bool dac_enabled_flag = (m_Context.channel1_volume & 0xF8) != 0;
		bool enable_channel_flag = (value & 0x80) == 0x80;
		if (enable_channel_flag && dac_enabled_flag)
		{
			// m_Context.audio_master |= 0x1;
		}
	}

	// Channel 2
	if (address == 0xFF16)
	{
		m_Context.channel2_length = value;
		m_LengthCounter2 = 63 - (value & 0x3F);
	}
	else if (address == 0xFF17)
	{
		m_Context.channel2_volume = value;
	}
	else if (address == 0xFF18)
	{
		m_Context.channel2_periodlow = value;
	}
	else if (address == 0xFF19)
	{
		m_Context.channel2_periodhigh = value;

		bool dac_enabled_flag = (m_Context.channel2_volume & 0xF8) != 0;
		bool enable_channel_flag = (value & 0x80) == 0x80;
		if (enable_channel_flag && dac_enabled_flag)
		{
			// m_Context.audio_master |= 0x2;
		}
	}

	// Channel 3
	if (address == 0xFF1A)
	{
		m_Context.channel3_dac_enable = value;
	}
	else if (address == 0xFF1B)
	{
		m_Context.channel3_length = value;
		m_LengthCounter3 = 63 - (value & 0x3F);
	}
	else if (address == 0xFF1C)
	{
		m_Context.channel3_output_level = value;
	}
	else if (address == 0xFF1D)
	{
		m_Context.channel3_perioidlow = value;
	}
	else if (address == 0xFF1E)
	{
		m_Context.channel3_perioidhigh = value;

		bool dac_enabled_flag = (m_Context.channel3_dac_enable & 0x80) == 0x80;
		bool enable_channel_flag = (value & 0x80) == 0x80;
		if (enable_channel_flag && dac_enabled_flag)
		{
			// m_Context.audio_master |= 0x4;
		}
	}

	// Channel 4
	if (address == 0xFF20)
	{
		m_Context.channel4_length = value;
		m_LengthCounter4 = 63 - (value & 0x3F);
	}
	else if (address == 0xFF21)
	{
		m_Context.channel4_volume = value;
	}
	else if (address == 0xFF22)
	{
		m_Context.channel4_frequency = value;
	}
	else if (address == 0xFF23)
	{
		m_Context.channel4_control = value;

		bool dac_enabled_flag = (m_Context.channel4_volume & 0xF8) != 0;
		bool enable_channel_flag = (value & 0x80) == 0x80;
		if (enable_channel_flag && dac_enabled_flag)
		{
			// m_Context.audio_master |= 0x8;
		}
	}

	// Write wave pattern RAM
	if (address >= 0xFF30 && address <= 0xFF3F)
	{
		m_WavePatternRam[address - 0xFF30] = value;
	}
}

uint8_t Apu::Read(uint16_t address)
{
	// std::cout << "Read APU: 0x" << std::hex << address << '\n';

	if (address == 0xFF26)
	{
		return m_Context.audio_master | 0x70;
	}
	else if (address == 0xFF25)
	{
		return m_Context.sound_panning;
	}
	else if (address == 0xFF24)
	{
		return m_Context.master_volume;
	}

	// Channel 1
	if (address == 0xFF10)
	{
		return m_Context.channel1_sweep | 0x80;
	}
	else if (address == 0xFF11)
	{
		return m_Context.channel1_length | 0x3F;
	}
	else if (address == 0xFF12)
	{
		return m_Context.channel1_volume | 0x0;
	}
	else if (address == 0xFF13)
	{
		return m_Context.channel1_periodlow | 0xFF;
	}
	else if (address == 0xFF14)
	{
		return m_Context.channel1_periodhigh | 0xBF;
	}

	// Channel 2
	if (address == 0xFF16)
	{
		return m_Context.channel2_length | 0x3F;
	}
	else if (address == 0xFF17)
	{
		return m_Context.channel2_volume | 0x0;
	}
	else if (address == 0xFF18)
	{
		return m_Context.channel2_periodlow | 0xFF;
	}
	else if (address == 0xFF19)
	{
		return m_Context.channel2_periodhigh | 0xBF;
	}

	// Channel 3
	if (address == 0xFF1A)
	{
		return m_Context.channel3_dac_enable | 0x7F;
	}
	else if (address == 0xFF1B)
	{
		return m_Context.channel3_length | 0xFF;
	}
	else if (address == 0xFF1C)
	{
		return m_Context.channel3_output_level | 0x9F;
	}
	else if (address == 0xFF1D)
	{
		return m_Context.channel3_perioidlow | 0xFF;
	}
	else if (address == 0xFF1E)
	{
		return m_Context.channel3_perioidhigh | 0xBF;
	}

	// Channel 4
	if (address == 0xFF20)
	{
		return m_Context.channel4_length | 0xFF;
	}
	else if (address == 0xFF21)
	{
		return m_Context.channel4_volume;
	}
	else if (address == 0xFF22)
	{
		return m_Context.channel4_frequency;
	}
	else if (address == 0xFF23)
	{
		return m_Context.channel4_control | 0xBF;
	}

	// Write wave pattern RAM
	if (address >= 0xFF30 && address <= 0xFF3F)
	{
		return m_WavePatternRam[address - 0xFF30];
	}

	// PCM
	if (address == 0xFF76)
	{
		return m_Context.pcm12;
	}
	else if (address == 0xFF77)
	{
		return m_Context.pcm34;
	}

	return 0xFF;
}

bool Apu::IsAudioOn() const
{
	return (m_Context.audio_master & 0x80) == 0x80;
}
