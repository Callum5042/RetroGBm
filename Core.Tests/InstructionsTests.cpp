#include "CppUnitTest.h"
#include <Emulator.h>
#include <Instructions.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsTests)
	{
	public:
		TEST_METHOD(Nop_IncreaseProgramCounter)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->ProgramCounter = 0;

			// Act
			Op::Nop(emulator.GetContext());

			// Assert
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->ProgramCounter));
			Assert::AreEqual(0x4, static_cast<int>(emulator.GetContext()->cycles));
		}

		TEST_METHOD(LoadR8_LoadRegisterIntoRegister)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_E, 0x0);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_D, 0x5);

			// Act
			Op::LoadR8(emulator.GetContext(), RegisterType8::REG_E, RegisterType8::REG_D);

			// Assert
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_D)));
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_E)));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x5);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x15, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetCarryFlags)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xFF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xA);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x9, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0xA, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetZeroFlags)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xFF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetHalfCarryFlags)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultIsZero_SetZeroFlag)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x15);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x15, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultWillUnderflow_SetCarryFlag)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x16);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x16, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultWillUnderflowNibble_SetHalfCarryFlag)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x10);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(IncR8_IncrementRegisterBy1_CarryFlagIsIgnored)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x6, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(IncR8_ResultsOverflown_SetZeroFlag_IgnoreCarryFlag)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xFF);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, false);
			emulator.GetCpu()->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
		}

		TEST_METHOD(IncR8_ResultsOverflown_UnderflownNibble_SetHalfCarry)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xF);
			emulator.GetCpu()->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_CarryFlagIsIgnored)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x4, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_ResultIsZero_ZeroFlagIsSet)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_ResultUnderflowsNibble_SetHalfCarryFlag)
		{
			// Arrange
			Emulator emulator;
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}
	};
}
