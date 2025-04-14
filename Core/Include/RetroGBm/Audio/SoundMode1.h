#pragma once

#include "SoundModeBase.h"
#include "FrequencySweep.h"
#include "VolumeEnvelope.h"

struct SoundContext;

class SoundMode1 : public SoundModeBase
{
public:
	SoundMode1(SoundContext* context);
	virtual ~SoundMode1() = default;

	void Start() override;
	void Trigger() override;
	int TickChannel() override;

	void SetNr0(int value);
	void SetNr1(int value);
	void SetNr2(int value);
	void SetNr3(int value);
	void SetNr4(int value);

	int GetNr0();
	int GetNr1();
	int GetNr2();
	int GetNr3();
	int GetNr4();

	void Write(uint16_t address, uint8_t value) override;
	int Read(uint16_t address) override;

private:
	SoundContext* m_Context = nullptr;
	FrequencySweep m_FrequencySweep;
	VolumeEnvelope m_VolumeEnvelope;

	int m_FrequencyDivider = 0;
	int m_LastOutput = 0;
	int m_Tick = 0;

	int GetDuty();
	void ResetFrequencyDivider();
	void UpdateSweep();
};