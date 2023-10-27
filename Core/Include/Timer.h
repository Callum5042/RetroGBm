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
};