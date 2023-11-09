#pragma once

#include <cstdint>

class Cpu;

enum class JoypadButton
{
	A,
	B,
	Start,
	Select,
	Up,
	Down,
	Left,
	Right,
};

struct JoypadState
{
	bool a;
	bool b;
	bool start;
	bool select;
	bool up;
	bool down;
	bool left;
	bool right;
};

class Joypad
{
	Cpu* m_Cpu = nullptr;

public:
	Joypad();
	Joypad(Cpu* cpu);
	virtual ~Joypad() = default;

	void Write(uint8_t value);
	void SetJoypad(JoypadButton button, bool state);
	uint8_t GamepadGetOutput();

private:
	bool m_SelectButtons = false;
	bool m_SelectDPad = false;
	JoypadState m_JoypadState = {};
};