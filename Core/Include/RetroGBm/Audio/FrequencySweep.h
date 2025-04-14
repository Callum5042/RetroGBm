#pragma once

class FrequencySweep
{
public:
	FrequencySweep();
	virtual ~FrequencySweep() = default;

    void Start();
    void Trigger();

    void SetNr10(int value);
    void SetNr13(int value);
    void SetNr14(int value);

    int GetNr13() const { return m_Nr13; }
    int GetNr14() const { return m_Nr14; }

    void Tick();
    int Calculate();

    bool IsEnabled() const { return !m_Overflow; }

private:
    int m_Divider = 0;

    // Sweep parameters
    int m_SweepPace = 0;
    bool m_DirectionSubtract = false;
    int m_SweepStep = 0;

    // Current process variables
    int m_Tick = 0;
    int m_Timer = 0;

    // Frequenecy variables
    int m_ShadowFrequenecy = 0;
    bool m_Overflow = false;
    bool m_CounterEnabled = false;
    bool m_Negging = false;

    // Registers - TODO: Should come from Context??
    int m_Nr13 = 0;
    int m_Nr14 = 0;
};