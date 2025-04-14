#pragma once

class PolynomialCounter
{
public:
	PolynomialCounter();

	void SetRegister(int value);
	bool Tick();

private:
	int m_Tick = 0;
	int m_ShiftedDivisor = 0;
};