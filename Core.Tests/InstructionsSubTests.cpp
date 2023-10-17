#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsSubTests)
	{
	public:
		TEST_METHOD(SubN8_IncreaseCyclesBy8_IncreaseProgramCounterBy1_SubResultToRegA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x2;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x8);
			context.cpu->SetFlag(CpuFlag::Subtraction, false);

			// Act
			Op::SubN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x6, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SubN8_ResultIsZero_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x5;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x5);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::SubN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SubN8_ResultUnderFlowsNibble_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x1;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::SubN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xF, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SubN8_ResultUnderFlowsByte_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x2;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x1);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::SubN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xFF, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}