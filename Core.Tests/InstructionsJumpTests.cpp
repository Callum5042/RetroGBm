#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

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

		/// <summary>
		/// 
		/// </summary>
		
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
	};
}