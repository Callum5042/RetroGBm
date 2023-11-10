#pragma once

#include <cstdint>

class Cpu;

struct TimerContext
{
	uint16_t div;
	uint8_t tima;
	uint8_t tma;
	uint8_t tac;
};

class Timer
{
	Cpu* m_Cpu = nullptr;

public:
	Timer();
	Timer(Cpu* cpu);
	virtual ~Timer() = default;

	void Init();
	void Tick();

	uint8_t Read(uint16_t address);
	void Write(uint16_t address, uint8_t value);

	inline const TimerContext* GetContext() { return &m_Context; }

private:
	TimerContext m_Context = {};

	bool m_BitPreviousState = false;
	bool m_TimerHasOverflown = false;
	int m_TimerOverflowTicks = 0;
};