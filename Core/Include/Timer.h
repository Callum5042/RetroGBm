#pragma once

#include <cstdint>

struct TimerContext
{
	uint16_t div;
	uint8_t tima;
	uint8_t tma;
	uint8_t tac;
};

class Timer
{
public:
	Timer() = default;
	virtual ~Timer() = default;

	void Init();
	void Tick();

	uint8_t Read(uint16_t address);
	void Write(uint16_t address, uint8_t value);

private:
	TimerContext context;

	int m_TimerCount = 0;

	int m_DividerClocksToWait = 0;
	int m_TimerClocksToWait = 0;
	int m_TimerOverflowWaitCycles = 0;
	bool m_TimerOverflown = false;
};