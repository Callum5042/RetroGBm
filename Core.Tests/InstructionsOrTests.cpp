#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsOrTests)
	{
	public:
		TEST_METHOD(OrR8_IncreaseCyclesBy4_ORValues_SetFlagsToFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cpu = std::make_unique<Cpu>();
			context.cycles = 0;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x6);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x8);

			// Act
			Op::OrR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xE, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(OrR8_IncreaseCyclesBy4_ResultIsZero_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cpu = std::make_unique<Cpu>();
			context.cycles = 0;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x0);

			// Act
			Op::OrR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(OrN8_IncreaseCyclesBy8_ORValues_SetFlagsToFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();
			context.cartridge->data.push_back(0);
			context.cartridge->data.push_back(0x8);

			context.cpu->SetRegister(RegisterType8::REG_A, 0x6);

			// Act
			Op::OrN8(&context);

			// Assert
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(8, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xE, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}
