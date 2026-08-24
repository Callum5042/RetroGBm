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
	m_TimerHasOverflown = false;
	m_TimerReloading = false;
	m_TimerOverflowTicks = 0;
}

void Timer::Tick()
{
	// The reload lasts for one T-cycle. A bus write between this tick and the
	// next one therefore observes the reload, while later writes do not.
	m_TimerReloading = false;

	// Advance an overflow that was already pending before this tick. Doing
	// this before detecting a new falling edge gives every overflow exactly
	// four complete T-cycles of delay, including overflows caused by TAC/DIV
	// writes between timer ticks.
	if (m_TimerHasOverflown)
	{
		m_TimerOverflowTicks--;
		if (m_TimerOverflowTicks == 0)
		{
			m_TimerHasOverflown = false;
			m_TimerReloading = true;
			m_Context.tima = m_Context.tma;
			m_Cpu->RequestInterrupt(InterruptFlag::Timer);
		}
	}

	const bool previousTimerInput = GetTimerInputState();

	// Increment div every tick (The actual visible part of DIV exposed in the register increments at 64 m-cycles or 256 t-cycles)
	// The DIV register will increment at 16,384 Hz or 32,768 Hz while on double speed (CGB).
	m_Context.div++;
	HandleTimerInputChange(previousTimerInput);
}

bool Timer::GetTimerInputState() const
{
	if ((m_Context.tac & 0b100) == 0)
	{
		return false;
	}

	int selectedBit = 9;
	switch (m_Context.tac & 0b11)
	{
		case 0b01:
			selectedBit = 3;
			break;
		case 0b10:
			selectedBit = 5;
			break;
		case 0b11:
			selectedBit = 7;
			break;
	}

	return ((m_Context.div >> selectedBit) & 0x1) != 0;
}

void Timer::IncrementTima()
{
	m_Context.tima++;
	if (m_Context.tima == 0)
	{
		// TIMA stays at zero for four T-cycles before TMA is reloaded.
		m_TimerHasOverflown = true;
		m_TimerOverflowTicks = 4;
	}
}

void Timer::HandleTimerInputChange(bool previousState)
{
	if (previousState && !GetTimerInputState())
	{
		IncrementTima();
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
		{
			// Always reset to 0 when wrote to
			const bool previousTimerInput = GetTimerInputState();
			m_Context.div = 0;
			HandleTimerInputChange(previousTimerInput);
			break;
		}
		case 0xFF05:
		{
			// A TIMA write during the reload T-cycle is ignored. During the
			// preceding overflow delay it replaces TIMA and cancels the reload.
			if (!m_TimerReloading)
			{
				m_TimerHasOverflown = false;
				m_Context.tima = value;
			}
			break;
		}
		case 0xFF06:
		{
			m_Context.tma = value;

			// During the reload T-cycle, the newly written TMA value is the value
			// transferred to TIMA. Ordinary TMA writes do not modify TIMA.
			if (m_TimerReloading)
			{
				m_Context.tima = m_Context.tma;
			}

			break;
		}
		case 0xFF07:
		{
			const bool previousTimerInput = GetTimerInputState();
			m_Context.tac = value;
			HandleTimerInputChange(previousTimerInput);
			break;
		}
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
