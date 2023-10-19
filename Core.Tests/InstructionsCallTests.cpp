#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>
#include <Bus.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsCallTests)
	{
	public:

		TEST_METHOD(CallN16_IncreaseCyclesBy24_JumpToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->ProgramCounter = 0x5;

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x6] = 0xEE;
			context.cartridge->data[0x7] = 0xFF;

			// Act
			Op::CallN16(&context);

			// Assert
			Assert::AreEqual(24, context.cycles);
			Assert::AreEqual(0xFFEE, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 0);
			Assert::AreEqual(0x8, static_cast<int>(stack_low));

			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0x0, static_cast<int>(stack_high));
		}

		TEST_METHOD(CallN16FlagNotSet_ZeroFlagNotSet_IncreaseCyclesBy24_JumpToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->ProgramCounter = 0x5;

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x6] = 0xEE;
			context.cartridge->data[0x7] = 0xFF;

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::CallN16FlagNotSet(&context, CpuFlag::Zero);

			// Assert
			Assert::AreEqual(24, context.cycles);
			Assert::AreEqual(0xFFEE, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 0);
			Assert::AreEqual(0x8, static_cast<int>(stack_low));

			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0x0, static_cast<int>(stack_high));
		}

		TEST_METHOD(CallN16FlagNotSet_CarryFlagNotSet_IncreaseCyclesBy24_JumpToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->ProgramCounter = 0x5;

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x6] = 0xEE;
			context.cartridge->data[0x7] = 0xFF;

			context.cpu->SetFlag(CpuFlag::Carry, false);

			// Act
			Op::CallN16FlagNotSet(&context, CpuFlag::Carry);

			// Assert
			Assert::AreEqual(24, context.cycles);
			Assert::AreEqual(0xFFEE, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 0);
			Assert::AreEqual(0x8, static_cast<int>(stack_low));

			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0x0, static_cast<int>(stack_high));
		}

		TEST_METHOD(CallN16FlagNotSet_ZeroFlagSet_IncreaseCyclesBy12_IncreaseProgramCounterBy3)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->ProgramCounter = 0x5;

			context.cartridge->data.resize(1024 * 8);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x6] = 0xEE;
			context.cartridge->data[0x7] = 0xFF;

			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::CallN16FlagNotSet(&context, CpuFlag::Zero);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0x8, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(ReturnFlagNotSet_ZeroFlagSet_IncreaseProgramCounter)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::ReturnFlagNotSet(&context, CpuFlag::Zero);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(ReturnFlagNotSet_ZeroFlagNotSet_SetProgramCounterToStack_DecreaseStackPointer)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.high_ram[125] = 0xEE;
			context.high_ram[124] = 0xFF;
			context.cpu->StackPointer = 0xFFFE - 2;

			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::ReturnFlagNotSet(&context, CpuFlag::Zero);

			// Assert
			Assert::AreEqual(20, context.cycles);
			Assert::AreEqual(0xEEFF, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE, static_cast<int>(context.cpu->StackPointer));
		}

		TEST_METHOD(Rst_IncreaseCyclesBy16_JumpToAddress_StorePcOnStack)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->ProgramCounter = 0x5;

			// Act
			Op::Rst(&context, 0x20);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x20, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0xFFFE - 0x2, static_cast<int>(context.cpu->StackPointer));

			uint8_t stack_low = ReadFromBus(&context, context.cpu->StackPointer + 0);
			Assert::AreEqual(0x6, static_cast<int>(stack_low));

			uint8_t stack_high = ReadFromBus(&context, context.cpu->StackPointer + 1);
			Assert::AreEqual(0x0, static_cast<int>(stack_high));
		}
	};
}