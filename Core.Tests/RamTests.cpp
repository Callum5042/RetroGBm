#include "CppUnitTest.h"
#include <Ram.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(RamTests)
	{
	public:
		TEST_METHOD(WriteWorkRam_ReadWorkRam)
		{
			// Arrange
			Ram ram;

			// Act
			ram.WriteWorkRam(0xC000, 0xAA);
			uint8_t result = ram.ReadWorkRam(0xC000);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
		}

		TEST_METHOD(WriteHighRam_ReadHighRam)
		{
			// Arrange
			Ram ram;

			// Act
			ram.WriteHighRam(0xFF80, 0xAA);
			uint8_t result = ram.ReadHighRam(0xFF80);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
		}
	};
}
