#include "Timer.h"

void Timer::Init()
{
	context.div = 0xAC00;
	context.tima = 0x0;
	context.tma = 0x0;
	context.tac = 0xF8;
}

void Timer::Tick()
{
	uint16_t prev_div = context.div;
	context.div++;

	bool timer_update = false;
	switch (context.tac & (0b11))
	{
		case 0b00:
			timer_update = (prev_div & (1 << 9)) && !(context.div & (1 << 9));
			break;
		case 0b01:
			timer_update = (prev_div & (1 << 3)) && !(context.div & (1 << 3));
			break;
		case 0b10:
			timer_update = (prev_div & (1 << 5)) && !(context.div & (1 << 5));
			break;
		case 0b11:
			timer_update = (prev_div & (1 << 7)) && !(context.div & (1 << 7));
			break;
	}

	// Read bit 2
	bool timer_enabled = context.tac & (1 << 2);
	if (timer_enabled && timer_update)
	{
		context.tima++;

		if (context.tima == 0xFF)
		{
			context.tima = context.tma;
			// Application::Instance->m_Emulator->m_Cpu.RequestInterrupts(InterruptType::TIMER);
		}
	}
}

uint8_t Timer::Read(uint16_t address)
{
	switch (address)
	{
		case 0xFF04:
			// Only want to expose the high byte
			return (context.div >> 8);
		case 0xFF05:
			return context.tima;
		case 0xFF06:
			return context.tma;
		case 0xFF07:
			return context.tac;
	}

	return 0;
}

void Timer::Write(uint16_t address, uint8_t value)
{
	switch (address)
	{
		case 0xFF04:
			// Always reset to 0 when wrote to
			context.div = 0;
			break;
		case 0xFF05:
			context.tima = value;
			break;
		case 0xFF06:
			context.tma = value;
			break;
		case 0xFF07:
			context.tac = value;
			break;
	}
}