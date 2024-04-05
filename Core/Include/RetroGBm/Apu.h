#pragma once

#include <cstdint>

class Timer;

struct ApuContext
{
	uint8_t audio_master = 0xF1;
	uint8_t sound_panning = 0xF3;
	uint8_t master_volume = 0x77;

	// Channel 1
	uint8_t channel1_sweep = 0x80;
	uint8_t channel1_length = 0xBF;
	uint8_t channel1_volume = 0xF3;
	uint8_t channel1_periodlow = 0xFF;
	uint8_t channel1_periodhigh = 0xBF;

	// Channel 2
	uint8_t channel2_length = 0x3F;
	uint8_t channel2_volume = 0;
	uint8_t channel2_periodlow = 0xFF;
	uint8_t channel2_periodhigh = 0xBF;

	// Channel 3
	uint8_t channel3_dac_enable = 0x7F;
	uint8_t channel3_length = 0xFF;
	uint8_t channel3_output_level = 0x9F;
	uint8_t channel3_perioidlow = 0xFF;
	uint8_t channel3_perioidhigh = 0xBF;

	// Channel 4
	uint8_t channel4_length = 0xFF;
	uint8_t channel4_volume = 0;
	uint8_t channel4_frequency = 0;
	uint8_t channel4_control = 0xBF;

	// PCM
	uint8_t pcm12 = 0;
	uint8_t pcm34 = 0;
};

class Apu
{
	Timer* m_Timer = nullptr;

public:
	Apu(Timer* timer);
	virtual ~Apu() = default;

	void Init();
	void Tick(bool doublespeed);

	void Write(uint16_t address, uint8_t value);
	uint8_t Read(uint16_t address);

	inline ApuContext* GetContext() { return &m_Context; }

private:
	ApuContext m_Context = {};

	bool m_ApuBitset = false;
	uint16_t m_ApuDiv = 0;

	void IncrementApuTimer(bool doublespeed);
};