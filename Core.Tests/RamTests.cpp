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
	};
}
