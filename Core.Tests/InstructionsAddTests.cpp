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
			Assert::AreEqual(0xE4, static_cast<int>(result));

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
			context.cartridge->data[0x1] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x15);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

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
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x65, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);
		}

		TEST_METHOD(AddSP_IncreaseCyclesBy16_SetFlagZeroAndSubtractFalse_AddResultToSP)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(static_cast<uint8_t>(0));
			context.cartridge->data.push_back(static_cast<uint8_t>(1));
			context.cpu->SetRegister(RegisterType16::REG_SP, 0x2);

			context.cpu->SetFlag(CpuFlag::Zero, true);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddSP(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_SP);
			Assert::AreEqual(0x3, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
		}

		TEST_METHOD(AddSP_WillOverFlowNibble_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x1);
			context.cpu->SetRegister(RegisterType16::REG_SP, 0xF);

			context.cpu->SetFlag(CpuFlag::Zero, true);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::AddSP(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_SP);
			Assert::AreEqual(0x10, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(AddR16_IncreasedCyclesBy8_SetSubtractionFlagToFalse_AddResultsToHL)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetRegister(RegisterType16::REG_BC, 0x15);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x10);

			// Act
			Op::AddR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x25, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);
		}

		TEST_METHOD(AddR16_ResultOverflows_SetCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType16::REG_BC, 0xFFAA);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0xFFAA);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddR16_ResultOverflows3rdNibble_SetHalfCarryFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType16::REG_BC, 0xFFF);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x1);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t result = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x1000, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddCarryN8_CarryFlagNotSet_AddResultToRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x15);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);

			// Act
			Op::AddCarryN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x65, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
		}
	};
}