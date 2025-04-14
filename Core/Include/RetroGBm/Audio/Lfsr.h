#pragma once

class Lfsr
{
public:
    Lfsr();

    void Start();
    void Reset();

    int NextBit(bool width_7bit);

private:
    int m_Value = 0;
};