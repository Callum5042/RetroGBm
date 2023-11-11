#include "CppUnitTest.h"
#include <Display.h>
#include <Ppu.h>
#include <Cpu.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	class NullBus : public IBus
	{
	public:
		NullBus() = default;

		uint8_t ReadBus(uint16_t address) override { return 0; }
		void WriteBus(uint16_t address, uint8_t value) override { }
	};

	TEST_CLASS(PpuTests)
	{
	public:
		TEST_METHOD(Init_StartsInOAM_Has0DotTicks_RamAndBufferInitialisedAndResizedCorrectly)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);

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
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
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
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
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

		TEST_METHOD(Tick_65664Ticks_EnterVBlankMode)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 65664; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::VBlank), static_cast<int>(display.GetLcdMode()));
			Assert::IsTrue(static_cast<bool>(cpu.GetInterruptFlags() & InterruptFlag::VBlank));

			Assert::AreEqual(0, static_cast<int>(ppu.m_Context.dot_ticks));
			Assert::AreEqual(144, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(Tick_IsInVBlankMode_TickFor4560Dots_Resets)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			const_cast<DisplayContext*>(display.GetContext())->ly = 144;
			display.SetLcdMode(LcdMode::VBlank);

			// Act
			for (int i = 0; i < 4560; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.m_Context.dot_ticks));
			Assert::AreEqual(0, static_cast<int>(display.GetContext()->ly));
		}
	};
}
