#include "CppUnitTest.h"
#include <Display.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(DisplayTests)
	{
	public:
		TEST_METHOD(IsBackgroundEnabled_LcdcBit0NotSet_ReturnsFalse)
		{
			// Arrange
			Display display;
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b0;

			// Act
			bool result = display.IsBackgroundEnabled();

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(IsBackgroundEnabled_LcdcBit0Set_ReturnsTrue)
		{
			// Arrange
			Display display;
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b1;

			// Act
			bool result = display.IsBackgroundEnabled();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsObjectEnabled_LcdcBit2Set_ReturnsTrue)
		{
			// Arrange
			Display display;
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b10;

			// Act
			bool result = display.IsObjectEnabled();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsObjectEnabled_LcdcBit2NotSet_ReturnsFalse)
		{
			// Arrange
			Display display;
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b00;

			// Act
			bool result = display.IsObjectEnabled();

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(WriteToLYRegister_IgnoreWrites)
		{
			// Arrange
			Display display;
			display.GetContext()->ly = 22;

			// Act
			uint16_t ly_register_address = 0xFF44;
			display.Write(ly_register_address, 10);

			// Assert
			Assert::AreEqual(22, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(WriteToSTATRegister_IgnoreBit012Writes)
		{
			// Arrange
			Display display;
			display.GetContext()->stat = 0x0;

			// Act
			uint16_t stat_register_address = 0xFF41;
			display.Write(stat_register_address, 0xFF);

			// Assert
			Assert::AreEqual(0xF8, static_cast<int>(display.GetContext()->stat));
		}

		TEST_METHOD(GetLcdMode_LcdModeEnumMapToStatBit01)
		{
			// Arrange
			Display display;

			// Act
			display.GetContext()->stat = 0b00;
			LcdMode hblank = display.GetLcdMode();

			display.GetContext()->stat = 0b01;
			LcdMode vblank = display.GetLcdMode();

			display.GetContext()->stat = 0b10;
			LcdMode oam = display.GetLcdMode();

			display.GetContext()->stat = 0b11;
			LcdMode pixel_transfer = display.GetLcdMode();

			// Assert
			Assert::IsTrue(LcdMode::HBlank == hblank);
			Assert::IsTrue(LcdMode::VBlank == vblank);
			Assert::IsTrue(LcdMode::OAM == oam);
			Assert::IsTrue(LcdMode::PixelTransfer == pixel_transfer);
		}
	};
}