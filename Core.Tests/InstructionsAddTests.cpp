#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsAddTests)
	{
	public:
		TEST_METHOD(AddR8_IncreaseCyclesBy4_AddRegAndStoreResultInRegA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x5);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x15, static_cast<int>(result));
		}

		TEST_METHOD(AddR8_ZeroFlagIsSet_ResetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x5);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x15, static_cast<int>(result));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(AddR8_ResultIsZero_ZeroFlagIsSet)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x0);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(AddR8_SubtractFlagIsSet_SubtractFlagIsResetToFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x5);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x15, static_cast<int>(result));

			bool flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(AddR8_ResultIsOverflown_SetCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0xFA);
			context.cpu->SetRegister(RegisterType8::REG_B, 0xEA);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xE5, static_cast<int>(result));

			bool flag = context.cpu->GetFlag(CpuFlag::Carry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(AddR8_HalfCarryOccurred_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x6);
			context.cpu->SetRegister(RegisterType8::REG_B, 0xA);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::AddR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x10, static_cast<int>(result));

			bool flag = context.cpu->GetFlag(CpuFlag::HalfCarry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(AddN8_IncreaseCyclesBy8_IncreaseProgramCounterBy1_AddResultToRegA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x15);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x65, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);
		}

		TEST_METHOD(AddIndirectHL_IncreaseCyclesBy8_IncreaseProgramCounterBy1_AddResultToRegA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x15);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddIndirectHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x65, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);
		}
	};
}