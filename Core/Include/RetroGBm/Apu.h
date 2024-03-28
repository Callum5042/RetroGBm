#pragma once

#include <cstdint>

struct ApuContext
{
	uint8_t audio_master;
	uint8_t sound_panning;
	uint8_t master_volume;

	// Channel 1
	uint8_t channel1_sweep;
	uint8_t channel1_length;
	uint8_t channel1_volume;
	uint8_t channel1_periodlow;
	uint8_t channel1_periodhigh;

	// Channel 2
	uint8_t channel2_length;
	uint8_t channel2_volume;
	uint8_t channel2_periodlow;
	uint8_t channel2_periodhigh;

	// Channel 3
	uint8_t channel3_dac_enable;
	uint8_t channel3_length;
	uint8_t channel3_output_level;
	uint8_t channel3_perioidlow;
	uint8_t channel3_perioidhigh;

	// Channel 4
	uint8_t channel4_length;
	uint8_t channel4_volume;
	uint8_t channel4_frequency;
	uint8_t channel4_control;
};

class Apu
{
public:
	Apu();
	virtual ~Apu() = default;

	void Init();

	void Write(uint16_t address, uint8_t value);
	uint8_t Read(uint16_t address);

	inline ApuContext* GetContext() { return &m_Context; }

private:
	ApuContext m_Context;
};