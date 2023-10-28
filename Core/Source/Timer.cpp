#include "Timer.h"
#include "Emulator.h"

void Timer::Init()
{
	context.div = 0xAC00;
	context.tima = 0x0;
	context.tma = 0x0;
	context.tac = 0xF8;
}

void Timer::Tick()
{
	m_DividerClocksToWait--;
	if (m_DividerClocksToWait <= 0)
	{
		m_DividerClocksToWait = 256;
		context.div++;
	}
	
	bool timer_enabled = context.tac & (1 << 2);
	if (timer_enabled)
	{
		m_TimerClocksToWait--;
		if (m_TimerClocksToWait <= 0)
		{
			const int cpu_clock = 4 * 1024 * 1024;
			switch (context.tac & (0b11))
			{
				case 0b00:
					m_TimerClocksToWait = cpu_clock / 4096;
					break;
				case 0b10:
					m_TimerClocksToWait = cpu_clock / 262144;
					break;
				case 0b01:
					m_TimerClocksToWait = cpu_clock / 65536;
					break;
				case 0b11:
					m_TimerClocksToWait = cpu_clock / 16384;
					break;
			}

			context.tima++;
			if (context.tima == 0)
			{
				m_TimerOverflowWaitCycles = 8;
				m_TimerOverflown = true;
			}
		}
	}

	if (m_TimerOverflown)
	{
		m_TimerOverflowWaitCycles -= 1;
		if (m_TimerOverflowWaitCycles <= 0)
		{
			m_TimerOverflown = false;

			context.tima = context.tma;
			Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::Timer);
		}
	}

	//uint16_t prev_div = context.div;
	//context.div++;

	//bool timer_update = false;
	//m_TimerCount -=4;
	//if (m_TimerCount <= 0)
	//{
	//	const int cpu_clock = 4 * 1024 * 1024;
	//	switch (context.tac & (0b11))
	//	{
	//		case 0b00:
	//			m_TimerCount = cpu_clock / 4096;
	//			break;
	//		case 0b01:
	//			m_TimerCount = cpu_clock / 262144;
	//			break;
	//		case 0b10:
	//			m_TimerCount = cpu_clock / 65536;
	//			break;
	//		case 0b11:
	//			m_TimerCount = cpu_clock / 16384;
	//			break;
	//	}

	//	context.tima++;
	//	if (context.tima == 0xFF)
	//	{
	//		timer_update = true;
	//	}
	//}

	//// Read bit 2
	//bool timer_enabled = context.tac & (1 << 2);
	//if (timer_enabled && timer_update)
	//{
	//	timer_update = false;
	//	context.tima = context.tma;
	//	Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::Timer);
	//}
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