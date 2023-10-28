#pragma once

#include <cstdint>

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
public:
	Joypad() = default;
	virtual ~Joypad() = default;

	void Write(uint8_t value);
	void SetJoypad(JoypadButton button, bool state);
	uint8_t GamepadGetOutput();

private:
	bool m_SelectButtons;
	bool m_SelectDPad;
	JoypadState m_JoypadState;
};