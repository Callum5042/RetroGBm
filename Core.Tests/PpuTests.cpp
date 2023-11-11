#include "CppUnitTest.h"
#include <Display.h>
#include <Ppu.h>
#include <Cpu.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(PpuTests)
	{
	public:
		TEST_METHOD(Init_StartsInOAM_Has0DotTicks_RamAndBufferInitialisedAndResizedCorrectly)
		{
			// Arrange
			Cpu cpu;
			Display display;

			Ppu ppu(&cpu, &display);

			// Act
			ppu.Init();

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.m_Context.dot_ticks));

			Assert::AreEqual(0x8000, static_cast<int>(ppu.m_Context.video_ram.size()));
			Assert::AreEqual(static_cast<int>(144 * 160 * sizeof(uint32_t)), static_cast<int>(ppu.m_Context.video_buffer.size()));
		}

		TEST_METHOD(Tick_80Ticks_EnterPixelTransferMode)
		{
			// Arrange
			Cpu cpu;
			Display display;

			Ppu ppu(&cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 80; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::PixelTransfer), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(80, static_cast<int>(ppu.m_Context.dot_ticks));
		}

		TEST_METHOD(Tick_456Ticks_EnterOamMode_DotTicksReset_IncreaseDisplayLyRegister)
		{
			// Arrange
			Cpu cpu;
			Display display;

			Ppu ppu(&cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 456; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.m_Context.dot_ticks));
			Assert::AreEqual(1, static_cast<int>(display.GetContext()->ly));
		}
	};
}
