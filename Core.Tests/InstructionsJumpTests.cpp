#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>
#include <Bus.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsJumpTests)
	{
	public:
		TEST_METHOD(JumpN16_IncreaseCyclesBy16_ProgramCounterUpdatedToResult)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);

			// Act
			Op::JumpN16(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_ZeroFlagNotSet_IncreaseCyclesBy16_ChangeProgramCounterToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Zero, false);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_ZeroFlagSet_IncreaseCyclesBy12_IncreaseProgramCounterBy2)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Zero, false);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_CarryFlagNotSet_IncreaseCyclesBy16_ChangeProgramCounterToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Carry, false);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Carry, false);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_CarryFlagSet_IncreaseCyclesBy12_IncreaseProgramCounterBy2)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Carry, false);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}
		
		TEST_METHOD(JumpFlagN16_CarryFlagSet_IncreaseCyclesBy16_ChangeProgramCounterToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Carry, true);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_CarryFlagNotSet_IncreaseCyclesBy12_IncreaseProgramCounterBy2)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Carry, false);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Carry, true);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}
		
		TEST_METHOD(JumpFlagN16_ZeroFlagSet_IncreaseCyclesBy16_ChangeProgramCounterToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpFlagN16_ZeroFlagNotSet_IncreaseCyclesBy12_IncreaseProgramCounterBy2)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x50);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::JumpFlagN16(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpHL_IncreaseCyclesBy4_SetProgramCounter)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5020);

			// Act
			Op::JumpHL(&context);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(0x5020, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpRelativeN8_IncreaseCyclesBy12_SetProgramCounter)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x2;

			context.cpu->ProgramCounter = 0x5;

			// Act
			Op::JumpRelativeN8(&context);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0x8, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpRelativeFlagN8_ZeroFlagNotSet_IncreaseCyclesBy12_JumpToRelativeAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x5;

			context.cpu->ProgramCounter = 0x5;
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::JumpRelativeFlagN8(&context, CpuFlag::Zero, false);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0xB, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpRelativeFlagN8_ZeroFlagSet_IncreaseCyclesBy8_IncreaseProgramCounterBy1)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x5;

			context.cpu->ProgramCounter = 0x5;
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::JumpRelativeFlagN8(&context, CpuFlag::Zero, false);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x6, static_cast<int>(context.cpu->ProgramCounter));
		}

		/// <summary>
		/// 
		/// </summary>

		TEST_METHOD(JumpRelativeFlagN8_ZeroFlagSet_IncreaseCyclesBy12_JumpToRelativeAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x5;

			context.cpu->ProgramCounter = 0x5;
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::JumpRelativeFlagN8(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0xB, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(JumpRelativeFlagN8_ZeroFlagNotSet_IncreaseCyclesBy8_IncreaseProgramCounterBy1)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x5;

			context.cpu->ProgramCounter = 0x5;
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::JumpRelativeFlagN8(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x6, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(CallN16_IncreaseCyclesBy24)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0xEE;
			context.cartridge->data[0x1] = 0xFF;

			// Act
			Op::CallN16(&context);

			// Assert
			Assert::AreEqual(24, context.cycles);
			Assert::AreEqual(0xFFEE, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 2);
			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0xEE, static_cast<int>(stack_low));
			Assert::AreEqual(0xFF, static_cast<int>(stack_high));
		}

		TEST_METHOD(CallN16Condition_ZeroFlagSet_IncreaseCyclesBy24_WriteToStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0xEE;
			context.cartridge->data[0x1] = 0xFF;

			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::CallN16Condition(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(24, context.cycles);
			Assert::AreEqual(0xFFEE, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 2);
			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0xEE, static_cast<int>(stack_low));
			Assert::AreEqual(0xFF, static_cast<int>(stack_high));
		}

		TEST_METHOD(CallN16Condition_ZeroFlagNotSet_IncreaseCyclesBy12_DoNotWriteToStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0xEE;
			context.cartridge->data[0x1] = 0xFF;

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::CallN16Condition(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0x2, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}
	};
}