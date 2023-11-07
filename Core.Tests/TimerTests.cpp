#include "CppUnitTest.h"
#include <Timer.h>
#include <Cpu.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(TimerTests)
	{
	public:
		TEST_METHOD(Init_SetDefaults)
		{
			// Arrange
			Timer timer(nullptr);

			// Act
			timer.Init();

			// Assert
			Assert::AreEqual(0xAC00, static_cast<int>(timer.GetContext()->div));
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->tma));
			Assert::AreEqual(0xF8, static_cast<int>(timer.GetContext()->tac));
		}

		TEST_METHOD(Write_WriteToDiv_DivAlways0)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();

			// Act
			timer.Write(0xFF04, 0xA);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->div));
		}

		TEST_METHOD(Read_OnlyHighBytes)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();

			// Set div to binary: 1111111110011001
			const_cast<TimerContext*>(timer.GetContext())->div = 0xFF99;

			// Act
			uint8_t result = timer.Read(0xFF04);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Tick_DivIncreasedBy1EveryTick)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();
			const_cast<TimerContext*>(timer.GetContext())->div = 0;

			// Act
			timer.Tick();

			// Assert
			Assert::AreEqual(0x1, static_cast<int>(timer.GetContext()->div));
		}

		TEST_METHOD(Tick_TmaIncreases1Every1024Ticks_Plus1ForFirstTick_Tac100)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();
			timer.Write(0xFF07, 0b100);

			// Act
			for (int i = 0; i <= 1024; ++i)
			{
				timer.Tick();
			}

			// Assert
			Assert::AreEqual(0x2, static_cast<int>(timer.GetContext()->tima));
		}

		TEST_METHOD(Tick_TmaOverflown_ResetTimaToTma_TimerInterruptIsRequested)
		{
			// Arrange
			Cpu mockCpu;

			Timer timer(&mockCpu);
			timer.Init();
			timer.Write(0xFF07, 0b100);
			timer.Write(0xFF06, 0xA);
			timer.Write(0xFF05, 0xFE);

			// Act
			timer.Tick();

			// Assert
			Assert::AreEqual(0xA, static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(static_cast<int>(timer.GetContext()->tma), static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(static_cast<int>(InterruptFlag::Timer), static_cast<int>(mockCpu.GetInterruptFlags()));
		}
	};
}
