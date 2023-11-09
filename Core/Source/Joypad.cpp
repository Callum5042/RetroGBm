#include "Pch.h"
#include "Joypad.h"
#include "Cpu.h"
#include "Emulator.h"

Joypad::Joypad()
{
	m_Cpu = Emulator::Instance->GetCpu();
}

Joypad::Joypad(Cpu* cpu) : m_Cpu(cpu)
{
}

void Joypad::Write(uint8_t value)
{
	m_SelectButtons = value & 0x20;
	m_SelectDPad = value & 0x10;
}

void Joypad::SetJoypad(JoypadButton button, bool state)
{
	uint8_t previous_state = this->GamepadGetOutput() & 0b111;

	switch (button)
	{
		case JoypadButton::A:
			m_JoypadState.a = state;
			break;
		case JoypadButton::B:
			m_JoypadState.b = state;
			break;
		case JoypadButton::Start:
			m_JoypadState.start = state;
			break;
		case JoypadButton::Select:
			m_JoypadState.select = state;
			break;
		case JoypadButton::Up:
			m_JoypadState.up = state;
			break;
		case JoypadButton::Down:
			m_JoypadState.down = state;
			break;
		case JoypadButton::Left:
			m_JoypadState.left = state;
			break;
		case JoypadButton::Right:
			m_JoypadState.right = state;
			break;
	}

	// Request interrupt when any button has been pressed
	uint8_t current_state = this->GamepadGetOutput() & 0b111;
	if (current_state != previous_state)
	{
		for (int i = 0; i < 3; ++i)
		{
			// Button pressed state is 0
			if ((current_state & (1 << i)) == 0 && (previous_state & (1 << i)) == 1)
			{
				m_Cpu->RequestInterrupt(InterruptFlag::Joypad);
				break;
			}
		}
	}
}

uint8_t Joypad::GamepadGetOutput()
{
	// Set 1111 1111 by default (bit 4 and 5 are used by the emulator to set the button/dpad enabled state)
	uint8_t output = 0xFF;

	// Can only set the start/select/a/b buttons if bit 5 is set
	if (!m_SelectButtons)
	{
		output &= ~(1 << 5);
		if (m_JoypadState.start)
		{
			output &= ~(1 << 3);
		}
		if (m_JoypadState.select)
		{
			output &= ~(1 << 2);
		}
		if (m_JoypadState.a)
		{
			output &= ~(1 << 0);
		}
		if (m_JoypadState.b)
		{
			output &= ~(1 << 1);
		}
	}

	// Can only set the up/down/left/right buttons if bit 4 is set
	if (!m_SelectDPad)
	{
		output &= ~(1 << 4);
		if (m_JoypadState.left)
		{
			output &= ~(1 << 1);
		}
		if (m_JoypadState.right)
		{
			output &= ~(1 << 0);
		}
		if (m_JoypadState.up)
		{
			output &= ~(1 << 2);
		}
		if (m_JoypadState.down)
		{
			output &= ~(1 << 3);
		}
	}

	return output;
}