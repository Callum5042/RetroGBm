#pragma once

#include "SoundModeBase.h"
#include <vector>

struct SoundContext;

class SoundMode3 : public SoundModeBase
{
    SoundContext* m_Context = nullptr;

public:
	SoundMode3(SoundContext* context);

    int Read(uint16_t address) override;
    void Write(uint16_t address, uint8_t value) override;

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

    void Start() override;
    void Trigger() override;
    int TickChannel() override;

private:
    const int DmgWave[16] =
    {
        0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C,
        0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA
    };

    const int CgbWave[16] =
    {
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
    };

    std::vector<uint8_t> m_WaveRam;

    int m_Tick = 0;
    int m_FrequencyDivider = 0;
    int m_LastOutput = 0;
    int m_TicksSinceRead = 65536;
    int m_LastReadAddress = 0;
    int m_Buffer = 0;
    bool m_Triggered = false;

    int GetVolume();
    int GetWaveEntry();
    void ResetFreqDivider();
};