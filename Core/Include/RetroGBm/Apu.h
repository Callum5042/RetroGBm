#pragma once

#include "RetroGBm/Audio/SoundModeBase.h"
#include "RetroGBm/Audio/ISoundOutput.h"

#include <memory>

struct SoundContext
{
	uint8_t audio_master = 0xF1;
	uint8_t sound_panning = 0xF3;
	uint8_t master_volume = 0x77;

	// Channel 1
	uint8_t channel1_nr10 = 0x80;
	uint8_t channel1_nr11 = 0xBF;
	uint8_t channel1_nr12 = 0xF3;
	uint8_t channel1_nr13 = 0xFF;
	uint8_t channel1_nr14 = 0xBF;

	// Channel 2
	uint8_t channel2_nr21 = 0x3F;
	uint8_t channel2_nr22 = 0x0;
	uint8_t channel2_nr23 = 0xFF;
	uint8_t channel2_nr24 = 0xBF;

	// Channel 3
	uint8_t channel3_nr30 = 0x7F;
	uint8_t channel3_nr31 = 0xFF;
	uint8_t channel3_nr32 = 0x9F;
	uint8_t channel3_nr33 = 0xFF;
	uint8_t channel3_nr34 = 0xBF;

	// Channel 4
	uint8_t channel4_nr41 = 0xFF;
	uint8_t channel4_nr42 = 0;
	uint8_t channel4_nr43 = 0;
	uint8_t channel4_nr44 = 0xBF;

	// PCM
	uint8_t pcm12 = 0;
	uint8_t pcm34 = 0;
};

class Apu
{
	ISoundOutput* m_SoundOutput = nullptr;

public:
	Apu(ISoundOutput* output);
    virtual ~Apu() = default;

    void Tick();

    void Write(uint16_t address, uint8_t value);
    int Read(uint16_t address);

    void Start();
    void Stop();

	SoundContext Context = {};

private:
    std::unique_ptr<SoundModeBase> m_SoundChannels[4];
    bool m_Enabled = false;
};