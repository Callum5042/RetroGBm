#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/PolynomialCounter.h"

PolynomialCounter::PolynomialCounter()
{
}

void PolynomialCounter::SetRegister(int value)
{
	int clock_shift = (value >> 4);

	int divisor = 0;
	switch (value & 0b111)
	{
		case 0: divisor = 8; break;
		case 1: divisor = 16; break;
		case 2: divisor = 32; break;
		case 3: divisor = 48; break;
		case 4: divisor = 64; break;
		case 5: divisor = 80; break;
		case 6: divisor = 96; break;
		case 7: divisor = 112; break;
		default: throw std::invalid_argument("Invalid value for divisor");
	}

	m_ShiftedDivisor = (divisor << clock_shift);
	m_Tick = 1;
}

bool PolynomialCounter::Tick()
{
	m_Tick--;
	if (m_Tick == 0)
	{
		m_Tick = m_ShiftedDivisor;
		return true;
	}
	else
	{
		return false;
	}
}