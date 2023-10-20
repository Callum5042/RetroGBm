#include "CppUnitTest.h"
#include <Instructions.h>
#include <ExtendedInstructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(ExtendedInstructionsTests)
	{
	public:
		TEST_METHOD(SwapR8_SwapNibblesInRegister_UnsetFlags)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x83);

			context.cpu->SetFlag(CpuFlag::Zero, true);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::HalfCarry, true);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			CB::SwapR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x38, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SwapR8_ByteIs0_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);

			context.cpu->SetFlag(CpuFlag::Zero, false);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::HalfCarry, true);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			CB::SwapR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(Bit_TestBit1_Bit1IsNotSet_UnsetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xF);

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			CB::Bit(&context, 1, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(Bit_TestBit1_Bit1IsSet_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xD);

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			CB::Bit(&context, 1, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(Set_SetRegisterValue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xD);

			// Act
			CB::Set(&context, 1, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xF, static_cast<int>(result));
		}

		TEST_METHOD(Reset_SetRegisterValue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xF);

			// Act
			CB::Reset(&context, 1, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xD, static_cast<int>(result));
		}

		TEST_METHOD(RotateLeft_ResultWillCarry_SetCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xC0);

			// Act
			CB::RotateLeft(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x80, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}