#include "CppUnitTest.h"
#include <RetroGBm/Cheats.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheatsTests
{
	TEST_CLASS(CheatsTests)
	{
	public:
		TEST_METHOD(ParseGamesharkCode_ReturnsToken)
		{
			// Act
			GamesharkToken token = ParseGamesharkCode("01FB04D2");

			// Assert
			Assert::AreEqual(static_cast<uint8_t>(0x01), token.bank);
			Assert::AreEqual(static_cast<uint8_t>(0xFB), token.value);
			Assert::AreEqual(static_cast<uint16_t>(0xD204), token.address);
		}
	};
}