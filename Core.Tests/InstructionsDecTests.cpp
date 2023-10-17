#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsDecTests)
	{
	public:
		TEST_METHOD(DecR8_IncreaseCyclesBy4_DecreaseRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xD);
			context.cpu->SetFlag(CpuFlag::Subtraction, false);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xC, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsTrue(subtract_flag);

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(zero_flag);
		}

		TEST_METHOD(DecR8_RegIs01_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0x01);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x0, static_cast<int>(result));

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(zero_flag);
		}

		TEST_METHOD(DecR8_ValueIs10_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);
			context.cpu->SetFlag(CpuFlag::Zero, false);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xF, static_cast<int>(result));

			bool half_carry_flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsTrue(half_carry_flag);

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsTrue(subtract_flag);
		}

		TEST_METHOD(DecR8_ValueIsF_DoNotSetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xF);
			context.cpu->SetFlag(CpuFlag::Zero, false);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xE, static_cast<int>(result));

			bool half_carry_flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsFalse(half_carry_flag);
		}

		TEST_METHOD(DecR16_IncreaseCyclesBy8_DecreaseRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType16::REG_BC, 0x5);

			// Act
			Op::DecR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_BC);
			Assert::AreEqual(0x4, static_cast<int>(result));
		}
	};
}