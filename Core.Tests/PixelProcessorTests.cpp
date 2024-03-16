#include "CppUnitTest.h"
#include <PixelProcessor.h>
#include <Display.h>
#include <Cpu.h>
#include <Cartridge.h>

#include <cstdint>
#include <stdexcept>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	static void SetDisplayLcd(Display* display, bool on)
	{
		const uint16_t lcdc_address = 0xFF40;
		uint8_t current_value = display->Read(lcdc_address);

		const uint8_t value = (on ? (current_value | 0x80) : (current_value & ~0x80));
		display->Write(lcdc_address, value);
	}

	static PixelProcessor BuildPixelProcessor()
	{
		Cartridge cartridge;
		Cpu processor(&cartridge);

		Display display;
		return PixelProcessor(&display, &processor);
	}

	static PixelProcessor BuildPixelProcessor(Display* display)
	{
		Cartridge cartridge;
		Cpu processor(&cartridge);

		return PixelProcessor(display, &processor);
	}

	TEST_CLASS(PixelProcessorTests)
	{
	public:
		TEST_METHOD(IsVideoRamAccessable_LcdIsDisabled_VideoRamIsAccessable)
		{
			// Arrange
			Display display;
			display.GetContext()->lcdc = 0xFF;

			SetDisplayLcd(&display, false);
			display.SetLcdMode(LcdMode::OAM);

			// Act
			PixelProcessor ppu(&display);
			bool result = ppu.IsVideoRamAccessable();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsVideoRamAccessable_LcdIsEnabledAndLcdModeIsOAM_VideoRamIsAccessable)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::OAM);

			// Act
			PixelProcessor ppu(&display);
			bool result = ppu.IsVideoRamAccessable();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsVideoRamAccessable_LcdIsEnabledAndLcdModeIsHBlank_VideoRamIsAccessable)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::HBlank);

			// Act
			PixelProcessor ppu(&display);
			bool result = ppu.IsVideoRamAccessable();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsVideoRamAccessable_LcdIsEnabledAndLcdModeIsVBlank_VideoRamIsAccessable)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::VBlank);

			// Act
			PixelProcessor ppu(&display);
			bool result = ppu.IsVideoRamAccessable();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsVideoRamAccessable_LcdIsEnabledAndLcdModeIsPixelTransfer_VideoRamIsNotAccessable)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::PixelTransfer);

			// Act
			PixelProcessor ppu(&display);
			bool result = ppu.IsVideoRamAccessable();

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(WriteVideoRam_VramIsDisabled_IgnoreWrites)
		{
			const uint16_t base_address = 0x8000;

			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::HBlank);

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const uint8_t default_value = 0x56;
			ppu.WriteVideoRam(base_address, default_value);

			// Act
			display.SetLcdMode(LcdMode::PixelTransfer);
			const uint8_t value = 0xAA;
			ppu.WriteVideoRam(base_address, value);

			// Assert
			display.SetLcdMode(LcdMode::HBlank);
			uint8_t result = ppu.ReadVideoRam(base_address);

			Assert::AreEqual(default_value, result);
			Assert::AreNotEqual(value, result);
		}

		TEST_METHOD(WriteVideoRam_VramIsEnabled_WritesToVram)
		{
			const uint16_t base_address = 0x8000;

			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::OAM);

			// Act
			PixelProcessor ppu = BuildPixelProcessor(&display);
			const uint8_t value = 0xAA;
			ppu.WriteVideoRam(base_address, value);

			// Assert
			uint8_t result = ppu.ReadVideoRam(base_address);
			Assert::AreEqual(value, result);
		}

		TEST_METHOD(WriteVideoRam_WritesToOutOfRangeAddress_ThrowsException)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::OAM);

			// Act
			PixelProcessor ppu = BuildPixelProcessor(&display);
			const uint8_t value = 0xAA;

			// Assert
			Assert::ExpectException<std::runtime_error>([&]
			{
				ppu.WriteVideoRam(0x0, value);
			});
		}

		TEST_METHOD(WriteVideoRam_ReadsFromOutOfRangeAddress_ThrowsException)
		{
			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::OAM);

			// Act
			PixelProcessor ppu = BuildPixelProcessor(&display);
			const uint8_t value = 0xAA;

			// Assert
			Assert::ExpectException<std::runtime_error>([&]
			{
				ppu.ReadVideoRam(0x55);
			});
		}

		TEST_METHOD(SetVideoRamBank_OnlyReadsBit0_VideoRamBankIs1)
		{
			// Act
			PixelProcessor ppu = BuildPixelProcessor();
			ppu.SetVideoRamBank(0x3);

			// Assert
			uint8_t result = ppu.GetContext()->video_ram_bank;
			Assert::AreEqual(0x1, static_cast<int>(result));
		}

		TEST_METHOD(GetVideoRamBank_ResultIs0xFF)
		{
			// Arrange
			PixelProcessor ppu = BuildPixelProcessor();
			ppu.SetVideoRamBank(0x1);

			// Act
			uint8_t result = ppu.GetVideoRamBank();

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(WriteVideoRam_VramBankIs1_WritesToBankedMemory)
		{
			const uint16_t base_address = 0x8000;

			// Arrange
			Display display;
			SetDisplayLcd(&display, true);
			display.SetLcdMode(LcdMode::OAM);

			PixelProcessor ppu = BuildPixelProcessor(&display);
			ppu.SetVideoRamBank(1);

			// Act
			const uint8_t value = 0xAB;
			ppu.WriteVideoRam(base_address, value);

			// Assert
			uint8_t bank = ppu.GetVideoRamBank();
			Assert::AreEqual(0xFF, static_cast<int>(bank));

			uint8_t result = ppu.ReadVideoRam(base_address);
			Assert::AreEqual(value, result);

			ppu.SetVideoRamBank(0);
			uint8_t unbanked_result = ppu.ReadVideoRam(base_address);
			Assert::AreNotEqual(value, unbanked_result);
		}

		TEST_METHOD(Tick_IncreaseDotCount)
		{
			// Arrange
			PixelProcessor ppu = BuildPixelProcessor();

			// Act
			ppu.Tick();
			
			// Assert
			Assert::AreEqual(1, ppu.GetContext()->dots);
		}

		TEST_METHOD(Tick_CheckLYCStatFlag_SetStatRegister)
		{
			// Arrange
			Display display;
			display.GetContext()->ly = 5;
			display.GetContext()->lyc = 5;

			PixelProcessor ppu = BuildPixelProcessor(&display);

			// Act
			ppu.Tick();

			// Assert
			Assert::AreEqual(0b100, static_cast<int>(display.GetContext()->stat));
		}

		TEST_METHOD(Tick_ModeIsOAMAndDotsAre79_ChangeToPixelTransferOn80thDot)
		{
			// Arrange
			Display display;
			display.SetLcdMode(LcdMode::OAM);

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const_cast<PixelProcessorContext*>(ppu.GetContext())->dots = 79;

			// Act
			ppu.Tick();

			// Assert
			Assert::IsTrue(display.GetLcdMode() == LcdMode::PixelTransfer);
			Assert::AreEqual(0, ppu.GetContext()->dots);
		}

		TEST_METHOD(Tick_ModeIsHBlankAndNotLastLine_UpdateLinesAndChangeToOAM)
		{
			// Arrange
			Display display;
			display.SetLcdMode(LcdMode::HBlank);
			display.GetContext()->ly = 5;

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const_cast<PixelProcessorContext*>(ppu.GetContext())->frame_dots = 455;

			// Act
			ppu.Tick();

			// Assert
			Assert::IsTrue(display.GetLcdMode() == LcdMode::OAM);
			Assert::AreEqual(0, ppu.GetContext()->dots);
			Assert::AreEqual(6, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(Tick_ModeIsHBlankAndLineIs143_UpdateLinesAndChangeToVBlank)
		{
			// Arrange
			Display display;
			display.SetLcdMode(LcdMode::HBlank);
			display.GetContext()->ly = 143;

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const_cast<PixelProcessorContext*>(ppu.GetContext())->frame_dots = 455;

			// Act
			ppu.Tick();

			// Assert
			Assert::IsTrue(display.GetLcdMode() == LcdMode::VBlank);
			Assert::AreEqual(0, ppu.GetContext()->dots);
			Assert::AreEqual(144, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(TickModeIsHBlank_WindowIsEnabled_UpdateWindowLine)
		{
			// Arrange
			Display display;
			display.SetLcdMode(LcdMode::HBlank);
			display.GetContext()->ly = 5;
			display.GetContext()->lcdc = (1 << 5);
			display.GetContext()->wy = 0;

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const_cast<PixelProcessorContext*>(ppu.GetContext())->frame_dots = 455;
			const_cast<PixelProcessorContext*>(ppu.GetContext())->window_line = 1;

			// Act
			ppu.Tick();

			// Assert
			Assert::IsTrue(display.GetLcdMode() == LcdMode::OAM);
			Assert::AreEqual(0, ppu.GetContext()->dots);
			Assert::AreEqual(2, static_cast<int>(ppu.GetContext()->window_line));
			Assert::AreEqual(6, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(TickModeIsVBlank_LastDotBeforeChange_ChangeModeToOam)
		{
			// Arrange
			Display display;
			display.SetLcdMode(LcdMode::VBlank);
			display.GetContext()->ly = 153;

			PixelProcessor ppu = BuildPixelProcessor(&display);
			const_cast<PixelProcessorContext*>(ppu.GetContext())->dots = 4560 - 1;
			const_cast<PixelProcessorContext*>(ppu.GetContext())->window_line = 143;

			// Act
			ppu.Tick();

			// Assert
			Assert::IsTrue(display.GetLcdMode() == LcdMode::OAM);
			Assert::AreEqual(0, ppu.GetContext()->dots);
			Assert::AreEqual(0, ppu.GetContext()->window_line);
			Assert::AreEqual(0, static_cast<int>(display.GetContext()->ly));
		}
	};
}
