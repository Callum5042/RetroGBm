#include "RetroGBm/Pch.h"
#include "RetroGBm/Timer.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cpu.h"

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
	// Increment div every tick (The actual visible part of DIV exposed in the register increments at 64 m-cycles or 256 t-cycles)
	// The DIV register will increment at 16,384 Hz or 32,768 Hz while on double speed (CGB).
	m_Context.div++;

	// Detect falling edge
	bool bitstate = false;
	switch (m_Context.tac & (0b11))
	{
		case 0b00:
			bitstate = (m_Context.div >> 9) & 0x1;
			break;
		case 0b01:
			bitstate = (m_Context.div >> 3) & 0x1;
			break;
		case 0b10:
			bitstate = (m_Context.div >> 5) & 0x1;
			break;
		case 0b11:
			bitstate = (m_Context.div >> 7) & 0x1;
			break;
	}

	bool timer_update = !bitstate && m_BitPreviousState;
	m_BitPreviousState = bitstate;

	// Increment tima from tac
	bool timer_enabled = (m_Context.tac >> 2) & 0x1;
	if (timer_enabled && timer_update)
	{
		m_Context.tima++;
		if (m_Context.tima == 0x0)
		{
			// Have to delay the interrupt and resetting the tima by 4 ticks
			m_TimerHasOverflown = true;
			m_TimerOverflowTicks = 4;
		}
	}

	// Handle overflow
	if (m_TimerHasOverflown)
	{
		m_TimerOverflowTicks--;
		if (m_TimerOverflowTicks < 0)
		{
			m_TimerHasOverflown = false;
			m_Context.tima = m_Context.tma;
			m_Cpu->RequestInterrupt(InterruptFlag::Timer);
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
			return (m_Context.tac | 0xF8);
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
		{
			// Writes to tima is ignored if the TMA was going to be loaded in the same cycle
			bool tmaBeingLoaded = m_TimerHasOverflown && m_TimerOverflowTicks == 0;
			if (!tmaBeingLoaded)
			{
				m_TimerHasOverflown = false;
				m_Context.tima = value;
			}
			break;
		}
		case 0xFF06:
		{
			m_Context.tma = value;

			// If TMA is being loaded in same cycle then also load tma as tima
			bool tmaBeingLoaded = m_TimerHasOverflown && m_TimerOverflowTicks == 0;
			if (!tmaBeingLoaded)
			{
				m_Context.tima = m_Context.tma;
			}

			break;
		}
		case 0xFF07:
			m_Context.tac = value;
			break;
	}
}

void Timer::SaveState(std::fstream* file)
{
	file->write(reinterpret_cast<const char*>(&m_Context), sizeof(TimerContext));
}

void Timer::LoadState(std::fstream* file)
{
	std::ifstream of("timer.bin", std::ios::in | std::ios::binary);
	file->read(reinterpret_cast<char*>(&m_Context), sizeof(TimerContext));
}