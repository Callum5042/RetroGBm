#include "CppUnitTest.h"
#include "MockCartridge.h"
#include <RetroGBm/Display.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(DisplayTests)
	{
	public:
		TEST_METHOD(IsBackgroundEnabled_LcdcBit0NotSet_ReturnsFalse)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b0;

			// Act
			bool result = display.IsBackgroundEnabled();

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(IsBackgroundEnabled_LcdcBit0Set_ReturnsTrue)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b1;

			// Act
			bool result = display.IsBackgroundEnabled();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsObjectEnabled_LcdcBit2Set_ReturnsTrue)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b10;

			// Act
			bool result = display.IsObjectEnabled();

			// Assert
			Assert::IsTrue(result);
		}

		TEST_METHOD(IsObjectEnabled_LcdcBit2NotSet_ReturnsFalse)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0b00;

			// Act
			bool result = display.IsObjectEnabled();

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(Write_StatRegisterIgnoresLower3Bits_StatIsF8)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->stat = 0;

			// Act
			display.Write(0xFF41, 0xFF);

			// Assert
			int result = const_cast<DisplayContext*>(display.GetContext())->stat;
			Assert::AreEqual(0xF8, result);
		}

		TEST_METHOD(Read_StatRegister_AlwaysSetLastBit)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->stat = 0;

			// Act
			int result = display.Read(0xFF41);

			// Assert
			Assert::AreEqual(0x80, result);
		}

		TEST_METHOD(Read_StatRegister_DoesNotOverwriteFirst3Bits)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->stat = 0b111;

			// Act
			display.Write(0xFF41, 0b1000);

			// Assert
			int result = const_cast<DisplayContext*>(display.GetContext())->stat;
			Assert::AreEqual(0x8F, result);
		}

		TEST_METHOD(Write_LyRegister_IgnoreWrites)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->ly = 0;

			// Act
			display.Write(0xFF44, 0xFF);

			// Assert
			int result = const_cast<DisplayContext*>(display.GetContext())->ly;
			Assert::AreEqual(0, result);
		}

		TEST_METHOD(Write_LcdcRegister_TurnsLcdOff_ResetsLyRegister)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0;
			const_cast<DisplayContext*>(display.GetContext())->ly = 50;

			// Act
			display.Write(0xFF40, 0);

			// Assert
			int result = const_cast<DisplayContext*>(display.GetContext())->ly;
			Assert::AreEqual(0, result);
		}

		TEST_METHOD(Read_StatRegister_LcdIsOff_Lower2BitsAre0)
		{
			// Arrange
			MockCartridge cartridge;
			Display display(&cartridge);
			display.Init();

			const_cast<DisplayContext*>(display.GetContext())->lcdc = 0;
			const_cast<DisplayContext*>(display.GetContext())->stat = 0x83;

			// Act
			int result = display.Read(0xFF41);

			// Assert
			Assert::AreEqual(0x80, result);
		}
	};
}