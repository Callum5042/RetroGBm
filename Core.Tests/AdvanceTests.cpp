#include "CppUnitTest.h"
#include "NullDisplayOutput.h"
#include "NullNetworkOutput.h"
#include "NullSoundOutput.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cpu.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	namespace
	{
		// ROM only cartridge filled with NOPs
		struct TestEmulator
		{
			NullDisplayOutput display_output;
			NullSoundOutput sound_output;
			NullNetworkOutput network_output;
			Emulator emulator{ &display_output, &sound_output, &network_output };

			TestEmulator()
			{
				emulator.SetFramePacingEnabled(false);
				emulator.LoadRom(std::vector<uint8_t>(0x8000, 0x00));
			}
		};
	}

	TEST_CLASS(AdvanceTests)
	{
	public:
		TEST_METHOD(StepAdvance_Paused_RunsOneInstructionThenStaysPaused)
		{
			// Arrange
			TestEmulator test;
			Emulator& emulator = test.emulator;
			emulator.Pause(true);
			const uint16_t pc = emulator.GetCpu()->ProgramCounter;

			// Act
			emulator.StepAdvance();
			emulator.Tick();
			emulator.Tick();

			// Assert - one NOP ran, the second Tick did nothing
			Assert::AreEqual(static_cast<int>(pc + 1), static_cast<int>(emulator.GetCpu()->ProgramCounter));
			Assert::IsTrue(emulator.IsPaused());
		}

		TEST_METHOD(StepAdvance_NotPaused_Ignored)
		{
			// Arrange
			TestEmulator test;
			Emulator& emulator = test.emulator;

			// Act
			emulator.StepAdvance();
			emulator.Pause(true);
			const uint16_t pc = emulator.GetCpu()->ProgramCounter;
			emulator.Tick();

			// Assert
			Assert::AreEqual(static_cast<int>(pc), static_cast<int>(emulator.GetCpu()->ProgramCounter));
		}

		TEST_METHOD(FrameAdvance_Paused_RunsToStartOfVBlankThenStaysPaused)
		{
			// Arrange
			TestEmulator test;
			Emulator& emulator = test.emulator;
			emulator.Pause(true);

			// Act - each Tick is one instruction; a frame is well under 20000 NOPs
			emulator.FrameAdvance();
			int ticks = 0;
			uint16_t pc = emulator.GetCpu()->ProgramCounter;
			for (; ticks < 20000; ++ticks)
			{
				emulator.Tick();
				const uint16_t new_pc = emulator.GetCpu()->ProgramCounter;
				if (new_pc == pc)
				{
					break;
				}

				pc = new_pc;
			}

			// Assert - stopped at line 144, the first line of VBlank
			Assert::IsTrue(ticks > 1);
			Assert::IsTrue(ticks < 20000);
			Assert::AreEqual(144, static_cast<int>(emulator.ReadBus(0xFF44)));
			Assert::IsTrue(emulator.IsPaused());
		}

		TEST_METHOD(FrameAdvance_Twice_RunsOneWholeFrame)
		{
			// Arrange
			TestEmulator test;
			Emulator& emulator = test.emulator;
			emulator.Pause(true);

			auto advance_frame = [&emulator]
			{
				emulator.FrameAdvance();
				int ticks = 0;
				uint16_t pc = emulator.GetCpu()->ProgramCounter;
				for (; ticks < 20000; ++ticks)
				{
					emulator.Tick();
					const uint16_t new_pc = emulator.GetCpu()->ProgramCounter;
					if (new_pc == pc)
					{
						break;
					}

					pc = new_pc;
				}

				return ticks;
			};

			// Act
			advance_frame();
			const int ticks = advance_frame();

			// Assert - a whole frame is 17556 cycles; by now the PC has run past the NOPs into
			// VRAM so a few instructions take longer than one cycle
			Assert::IsTrue(ticks > 17000);
			Assert::IsTrue(ticks <= 17556);
			Assert::AreEqual(144, static_cast<int>(emulator.ReadBus(0xFF44)));
		}
	};
}
