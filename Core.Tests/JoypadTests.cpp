#include "CppUnitTest.h"
#include <Joypad.h>
#include <Cpu.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(JoypadTests)
	{
	public:
		TEST_METHOD(SetJoypad_KeyPressed_RequestInterrupt)
		{
			// Arrange
			Cpu mockCpu;

			Joypad joypad(&mockCpu);
			joypad.SetJoypad(JoypadButton::A, false);

			// Act
			joypad.SetJoypad(JoypadButton::A, true);

			// Assert
			bool flagset = (mockCpu.GetInterruptFlags() & InterruptFlag::Joypad) == InterruptFlag::Joypad;
			Assert::IsTrue(flagset);
		}

		TEST_METHOD(SetJoypad_KeyNotPressed_DoNotRequestInterrupt)
		{
			// Arrange
			Cpu mockCpu;

			Joypad joypad(&mockCpu);
			joypad.SetJoypad(JoypadButton::A, false);

			// Act
			joypad.SetJoypad(JoypadButton::A, false);

			// Assert
			bool flagset = (mockCpu.GetInterruptFlags() & InterruptFlag::Joypad) == InterruptFlag::Joypad;
			Assert::IsFalse(flagset);
		}

		TEST_METHOD(GamepadGetOutput_KeyIsPressed_OutputIsEquals)
		{
			// Arrange
			Cpu mockCpu;

			Joypad joypad(&mockCpu);
			joypad.Write(0x10);
			joypad.SetJoypad(JoypadButton::A, true);

			// Act
			uint8_t result = joypad.GamepadGetOutput();

			// Assert
			uint8_t correct_state = 0xDE;
			Assert::AreEqual(result, correct_state);
		}
	};
}
