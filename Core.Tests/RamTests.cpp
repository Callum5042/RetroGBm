#include "CppUnitTest.h"
#include <RetroGBm/Ram.h>

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

		TEST_METHOD(SetWorkRamBank_OnlyLower3BitsRead)
		{
			// Arrange
			Ram ram;

			// Act
			ram.SetWorkRamBank(15);

			// Assert
			Assert::AreEqual(7, static_cast<int>(ram.GetWorkRamBank()));
		}

		TEST_METHOD(SetWorkRamBank_AttemptToZero_SetsToOne)
		{
			// Arrange
			Ram ram;

			// Act
			ram.SetWorkRamBank(0);

			// Assert
			Assert::AreEqual(1, static_cast<int>(ram.GetWorkRamBank()));
		}

		TEST_METHOD(WriteWorkRam_InCFFFRange_IgnoreBankValue_ReadWorkRam)
		{
			// Arrange
			Ram ram;
			ram.SetWorkRamBank(2);

			// Act
			ram.WriteWorkRam(0xC000, 0xAA);
			uint8_t result = ram.ReadWorkRam(0xC000);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
		}

		TEST_METHOD(WriteWorkRam_InDFFFRange_BankValue_ReadWorkRam)
		{
			// Arrange
			Ram ram;
			ram.SetWorkRamBank(1);

			// Act
			ram.WriteWorkRam(0xD000, 0xAA);
			uint8_t result = ram.ReadWorkRam(0xD000);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
			Assert::AreEqual(0xAA, static_cast<int>(ram.GetWorkRam()[4096]));
		}

		TEST_METHOD(WriteWorkRam_InDFFFRange_BankValue_AddressEndOfBank1_ReadWorkRam)
		{
			// Arrange
			Ram ram;
			ram.SetWorkRamBank(1);

			// Act
			ram.WriteWorkRam(0xDFFF, 0xAA);
			uint8_t result = ram.ReadWorkRam(0xDFFF);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
			Assert::AreEqual(0xAA, static_cast<int>(ram.GetWorkRam()[8191]));
		}

		TEST_METHOD(WriteWorkRam_InDFFFRange_BankValue2_ReadWorkRam)
		{
			// Arrange
			Ram ram;
			ram.SetWorkRamBank(2);

			// Act
			ram.WriteWorkRam(0xD000, 0xAA);
			uint8_t result = ram.ReadWorkRam(0xD000);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
			Assert::AreEqual(0xAA, static_cast<int>(ram.GetWorkRam()[8192]));
		}

		TEST_METHOD(ReadEchoRam_ReadsFromWorkRam)
		{
			// Arrange
			Ram ram;

			// Act
			ram.WriteWorkRam(0xC000, 0xAA);
			uint8_t result = ram.ReadEchoRam(0xE000);

			// Assert
			Assert::AreEqual(0xAA, static_cast<int>(result));
		}

		TEST_METHOD(PokeWorkRam_SetsABankThatIsNotMappedIn)
		{
			// Arrange
			Ram ram;
			ram.SetWorkRamBank(1);

			// Act: bank 3, $D010
			ram.PokeWorkRam(3 * 0x1000 + 0x10, 0x5A);
			ram.SetWorkRamBank(3);

			// Assert
			Assert::AreEqual(0x5A, static_cast<int>(ram.ReadWorkRam(0xD010)));
		}

		TEST_METHOD(PokeHighRam_SetsTheByteAtTheOffset)
		{
			// Arrange
			Ram ram;

			// Act
			ram.PokeHighRam(0x05, 0x5A);

			// Assert
			Assert::AreEqual(0x5A, static_cast<int>(ram.ReadHighRam(0xFF85)));
		}

		TEST_METHOD(Poke_PastTheEnd_IsIgnored)
		{
			// Arrange
			Ram ram;

			// Act
			ram.PokeWorkRam(ram.GetWorkRam().size(), 0x5A);
			ram.PokeHighRam(ram.GetHighRam().size(), 0x5A);

			// Assert
			Assert::AreEqual(size_t(0x8000), ram.GetWorkRam().size());
			Assert::AreEqual(size_t(0x80), ram.GetHighRam().size());
		}
	};
}
