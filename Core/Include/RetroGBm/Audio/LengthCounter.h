#pragma once

#include "RetroGBm/Constants.h"

class LengthCounter
{
public:
	LengthCounter(int full_length);
	virtual ~LengthCounter() = default;

	void Start();
	void Tick();

	void SetLength(int length);
	void SetRegister(int value);

	void Reset();

	inline bool IsEnabled() const { return m_Enabled; }
	inline int GetLength() const { return m_Length; }

private:
	const int m_Divider = GameBoy::TicksPerSec / 256;

	long m_Tick = 0;
	int m_FullLength = 0;


	bool m_Enabled = false;
	int m_Length = 0;
};