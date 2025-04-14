#pragma once

#include "SoundModeBase.h"
#include "VolumeEnvelope.h"
#include "PolynomialCounter.h"
#include "Lfsr.h"

struct SoundContext;

class SoundMode4 : public SoundModeBase
{
    SoundContext* m_Context = nullptr;

public:
	SoundMode4(SoundContext* context);

    void Start() override;
    void Trigger() override;
    int TickChannel() override;

    void SetNr1(int value);
    void SetNr2(int value);
    void SetNr3(int value);
    void SetNr4(int value);

    int GetNr1();
    int GetNr2();
    int GetNr3();
    int GetNr4();

    void Write(uint16_t address, uint8_t value) override;
    int Read(uint16_t address) override;

private:
    VolumeEnvelope m_VolumeEnvelope;
    PolynomialCounter m_PolynomialCounter;
    Lfsr m_Lfsr;

    int m_LastResult = 0;
};