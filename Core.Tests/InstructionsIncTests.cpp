#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsIncTests)
	{
	public:
		TEST_METHOD(IncR8_IncreaseCyclesBy4_IncreaseRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xA);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::IncR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xB, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(subtract_flag);
		}

		TEST_METHOD(IncR8_RegIsFF_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xFF);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::IncR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x0, static_cast<int>(result));

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(zero_flag);
		}

		TEST_METHOD(IncR8_RegIsF_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xF);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::IncR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x10, static_cast<int>(result));

			bool half_carry_flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsTrue(half_carry_flag);
		}

		TEST_METHOD(IncR8_RegIsE_DoNotSetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xE);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::IncR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xF, static_cast<int>(result));

			bool half_carry_flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsFalse(half_carry_flag);
		}

		TEST_METHOD(IncR8_RegIs10_DoNotSetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::IncR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x11, static_cast<int>(result));

			bool half_carry_flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsFalse(half_carry_flag);
		}

		TEST_METHOD(IncR16_IncreaseCyclesBy8_IncreaseRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType16::REG_BC, 0x5);

			// Act
			Op::IncR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_BC);
			Assert::AreEqual(0x6, static_cast<int>(result));
		}
	};
}