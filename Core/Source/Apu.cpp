#include "RetroGBm/Pch.h"
#include "RetroGBm/Apu.h"

#include <cstdint>

Apu::Apu()
{
}

void Apu::Init()
{
}

void Apu::Write(uint16_t address, uint8_t value)
{
	if (address == 0xFF26)
	{
		m_Context.audio_master = value;
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
	}

	// Channel 2
	if (address == 0xFF16)
	{
		m_Context.channel2_length = value;
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
	}

	// Channel 3
	if (address == 0xFF1A)
	{
		m_Context.channel3_dac_enable = value;
	}
	else if (address == 0xFF1B)
	{
		m_Context.channel3_length = value;
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
	}

	// Channel 4
	if (address == 0xFF20)
	{
		m_Context.channel4_length = value;
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
	}
}

uint8_t Apu::Read(uint16_t address)
{
	if (address == 0xFF26)
	{
		return m_Context.audio_master;
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
		return m_Context.channel1_sweep;
	}
	else if (address == 0xFF11)
	{
		return m_Context.channel1_length;
	}
	else if (address == 0xFF12)
	{
		return m_Context.channel1_volume;
	}
	else if (address == 0xFF13)
	{
		return m_Context.channel1_periodlow;
	}
	else if (address == 0xFF14)
	{
		return m_Context.channel1_periodhigh;
	}

	// Channel 2
	if (address == 0xFF16)
	{
		return m_Context.channel2_length;
	}
	else if (address == 0xFF17)
	{
		return m_Context.channel2_volume;
	}
	else if (address == 0xFF18)
	{
		return m_Context.channel2_periodlow;
	}
	else if (address == 0xFF19)
	{
		return m_Context.channel2_periodhigh;
	}

	// Channel 3
	if (address == 0xFF1A)
	{
		return m_Context.channel3_dac_enable;
	}
	else if (address == 0xFF1B)
	{
		return m_Context.channel3_length;
	}
	else if (address == 0xFF1C)
	{
		return m_Context.channel3_output_level;
	}
	else if (address == 0xFF1D)
	{
		return m_Context.channel3_perioidlow;
	}
	else if (address == 0xFF1E)
	{
		return m_Context.channel3_perioidhigh;
	}

	// Channel 4
	if (address == 0xFF20)
	{
		return m_Context.channel4_length;
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
		return m_Context.channel4_control;
	}

	return 0xFF;
}