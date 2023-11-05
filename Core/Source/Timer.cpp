#include "Pch.h"
#include "Timer.h"
#include "Emulator.h"
#include "Cpu.h"

Timer::Timer()
{
	m_Cpu = Emulator::Instance->GetCpu();
}

Timer::Timer(Cpu* cpu) : m_Cpu(cpu)
{
}

void Timer::Init()
{
	m_Context.div = 0xAC00;
	m_Context.tima = 0x0;
	m_Context.tma = 0x0;
	m_Context.tac = 0xF8;
}

void Timer::Tick()
{
	// Increment div every 256 ticks
	m_DividerClocksToWait += 4;
	if (m_DividerClocksToWait >= 256)
	{
		m_DividerClocksToWait = 0;
		m_Context.div++;
	}
	
	// Increment tima from tac
	bool timer_enabled = m_Context.tac & (1 << 2);
	if (timer_enabled)
	{
		m_TimerClocksToWait -= 4;
		if (m_TimerClocksToWait <= 0)
		{
			const int cpu_clock = 4 * 1024 * 1024;
			switch (m_Context.tac & (0b11))
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

			m_Context.tima++;
			if (m_Context.tima == 0xFF)
			{
				m_Context.tima = m_Context.tma;
				m_Cpu->RequestInterrupt(InterruptFlag::Timer);
			}
		}
	}
}

uint8_t Timer::Read(uint16_t address)
{
	switch (address)
	{
		case 0xFF04:
			// Only want to expose the high byte
			return (m_Context.div >> 8);
		case 0xFF05:
			return m_Context.tima;
		case 0xFF06:
			return m_Context.tma;
		case 0xFF07:
			return m_Context.tac;
	}

	return 0xFF;
}

void Timer::Write(uint16_t address, uint8_t value)
{
	switch (address)
	{
		case 0xFF04:
			// Always reset to 0 when wrote to
			m_Context.div = 0;
			break;
		case 0xFF05:
			m_Context.tima = value;
			break;
		case 0xFF06:
			m_Context.tma = value;
			break;
		case 0xFF07:
			m_Context.tac = value;
			break;
	}
}