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
	};
}