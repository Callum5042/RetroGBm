#include "CppUnitTest.h"
#include <Display.h>

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

		TEST_METHOD(Test)
		{
			// Arrange
			Display display;
			display.Init();

			display.Write(0xFF68, 0x80);
			// display.Write(0xFF69, 0x1F);

			// Red
			display.m_BackgroundColourPalettes[0] = 0x1F;
			display.m_BackgroundColourPalettes[1] = 0x0;

			// Green
			display.m_BackgroundColourPalettes[2] = 0x0;
			display.m_BackgroundColourPalettes[3] = 0x0;

			// Blue
			display.m_BackgroundColourPalettes[4] = 0x0;
			display.m_BackgroundColourPalettes[5] = 0x0;

			// Alpha?
			display.m_BackgroundColourPalettes[6] = 0x0;
			display.m_BackgroundColourPalettes[7] = 0x0;

			// Act
			uint32_t colour = display.GetColourFromBackgroundPalette(0, 0);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(colour));
		}
	};
}