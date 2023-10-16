#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>
#include <Bus.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
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

			context.cartridge->data.push_back(0x12);
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

			context.cpu->ProgramCounter = 0x4;

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

		TEST_METHOD(Return_IncreaseCyclesBy16_SetProgramCounter_DecreaseStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.high_ram[125] = 0xEE;
			context.high_ram[124] = 0xFF;

			context.cpu->StackPointer -= 2;

			// Act
			Op::Return(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0xEEFF, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(ReturnCondition_ZeroFlagSet_IncreaseCyclesBy20_SetProgramCounter_IncreaseStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.high_ram[125] = 0xEE;
			context.high_ram[124] = 0xFF;

			context.cpu->StackPointer -= 2;

			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::ReturnCondition(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(20, context.cycles);
			Assert::AreEqual(0xEEFF, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(ReturnCondition_ZeroFlagNotSet_IncreaseCyclesBy8)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.high_ram[125] = 0xEE;
			context.high_ram[124] = 0xFF;

			context.cpu->StackPointer -= 2;

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::ReturnCondition(&context, CpuFlag::Zero, true);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x0, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 2, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(PushR16_IncreaseCyclesBy16_PushRegToHighRam_IncreaseStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_C, 0x36);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x24);

			// Act
			Op::PushR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(16, context.cycles);

			Assert::AreEqual(0x24, static_cast<int>(context.high_ram[0xFFFD - 0xFF80]));
			Assert::AreEqual(0x36, static_cast<int>(context.high_ram[0xFFFC - 0xFF80]));
			Assert::AreEqual(0xFFFE - 2, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(PopR12_IncreaseCyclesBy16_ReadFromHighRam_DecreaseStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_C, 0x0);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x0);

			context.high_ram[0xFFFD - 0xFF80] = 0x36;
			context.high_ram[0xFFFC - 0xFF80] = 0x24;
			context.cpu->StackPointer = 0xFFFE - 2;

			// Act
			Op::PopR16(&context, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));

			Assert::AreEqual(0x36, static_cast<int>(context.cpu->GetRegister(RegisterType8::REG_B)));
			Assert::AreEqual(0x24, static_cast<int>(context.cpu->GetRegister(RegisterType8::REG_C)));
		}
	};
}