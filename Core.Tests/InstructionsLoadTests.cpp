#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsLoadTests)
	{
	public:
		TEST_METHOD(LoadR8_IncreaseCyclesBy4_SetRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::LoadR8(&context, RegisterType8::REG_A, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x10, static_cast<int>(result));
		}

		TEST_METHOD(LoadN8_IncreaseCyclesBy8_IncreaseProgramCounter_SetRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();
			context.cartridge->data.push_back(0x05);
			context.cartridge->data.push_back(0x10);

			// Act
			Op::LoadN8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x10, static_cast<int>(result));
		}

		TEST_METHOD(LoadN16_IncreaseCyclesBy12_SetRegisterToResult)
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
			Op::LoadN16(&context, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(3, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t reg = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x5020, static_cast<int>(reg));
		}

		TEST_METHOD(LoadN16_ForRegSP_IncreaseCyclesBy12_SetRegisterToResult)
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
			Op::LoadN16(&context, RegisterType16::REG_SP);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(3, static_cast<int>(context.cpu->ProgramCounter));

			uint16_t reg = context.cpu->GetRegister(RegisterType16::REG_SP);
			Assert::AreEqual(0x5020, static_cast<int>(reg));
		}

		TEST_METHOD(StoreIncrementHL_IncreaseCyclesBy8_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x20);

			context.cpu->SetRegister(RegisterType16::REG_HL, 0x1);
			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);

			// Act
			Op::StoreIncrementHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x10, static_cast<int>(context.cartridge->data[1]));

			uint16_t address = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x2, static_cast<int>(address));

			uint8_t address_data = context.cartridge->data[address];
			Assert::AreEqual(0x20, static_cast<int>(address_data));
		}

		TEST_METHOD(StoreDecrementHL_IncreaseCyclesBy8_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.push_back(0x20);
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x0);

			context.cpu->SetRegister(RegisterType16::REG_HL, 0x1);
			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);

			// Act
			Op::StoreDecrementHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x10, static_cast<int>(context.cartridge->data[1]));

			uint16_t address = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x0, static_cast<int>(address));

			uint8_t address_data = context.cartridge->data[address];
			Assert::AreEqual(0x20, static_cast<int>(address_data));
		}

		TEST_METHOD(StoreR8_IncreaseByCycleBy8_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);

			context.cpu->SetRegister(RegisterType16::REG_BC, 0x5);
			context.cpu->SetRegister(RegisterType8::REG_A, 0x50);

			// Act
			Op::StoreR8(&context, RegisterType8::REG_A, RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x50, static_cast<int>(context.cartridge->data[0x5]));
		}

		TEST_METHOD(StoreN8_IncreaseByCycleBy12_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0x50;

			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);

			// Act
			Op::StoreN8(&context, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(0x50, static_cast<int>(context.cartridge->data[0x5]));
		}

		TEST_METHOD(LoadIndirectR16_IncreaseCyclesBy8_SetRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);

			// Act
			Op::LoadIndirectR16(&context, RegisterType8::REG_B, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x50, static_cast<int>(context.cartridge->data[0x5]));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x50, static_cast<int>(result));
		}

		TEST_METHOD(StoreIndirectAC_IncreaseCycleBy8_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x1);
			context.cpu->SetRegister(RegisterType8::REG_C, 0x1);

			// Act
			Op::StoreIndirectAC(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(0x1, static_cast<int>(context.serial_data[0x0]));
		}

		TEST_METHOD(LoadIndirectAC_IncreaseCyclesBy8_SetRegisterA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.serial_data[0] = 0x1;
			context.cpu->SetRegister(RegisterType8::REG_C, 0x1);

			// Act
			Op::LoadIndirectAC(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x1, static_cast<int>(result));
		}

		TEST_METHOD(StoreIndirectA16_IncreaseCyclesby16)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_C, 0x5);

			// Act
			Op::StoreIndirectA16(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(LoadIndirectA16_IncreaseCyclesby16)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_C, 0x5);

			// Act
			Op::LoadIndirectA16(&context);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
		}

		TEST_METHOD(LoadIncrementHL_IncreaseCyclesBy8_IncrementHL_SetRegisterA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);

			// Act
			Op::LoadIncrementHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint8_t reg_a = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x50, static_cast<int>(reg_a));

			uint16_t reg_hl = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x6, static_cast<int>(reg_hl));
		}

		TEST_METHOD(LoadDecrementHL_IncreaseCyclesBy8_IncrementHL_SetRegisterA)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x50;

			context.cpu->SetRegister(RegisterType8::REG_A, 0);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);

			// Act
			Op::LoadDecrementHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint8_t reg_a = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x50, static_cast<int>(reg_a));

			uint16_t reg_hl = context.cpu->GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(0x4, static_cast<int>(reg_hl));
		}

		TEST_METHOD(StoreIndirectR8_IncreaseCyclesBy16_WriteToAddress)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0x0;
			context.cartridge->data[0x1] = 0x5;
			context.cartridge->data[0x5] = 0x1;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x32);

			// Act
			Op::StoreIndirectR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x3, static_cast<int>(context.cpu->ProgramCounter));

			Assert::AreEqual(0x32, static_cast<int>(context.cartridge->data[0x5]));
		}

		TEST_METHOD(LoadIndirectR8_IncreaseCyclesBy16_WriteToRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x0] = 0x5;
			context.cartridge->data[0x1] = 0x0;
			context.cartridge->data[0x5] = 0x32;

			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);

			// Act
			Op::LoadIndirectR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(16, context.cycles);
			Assert::AreEqual(0x2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x32, static_cast<int>(result));
		}

		TEST_METHOD(StoreFF00_IncreaseCyclesBy16_WriteToHighRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x1;

			// Act
			Op::StoreFF00(&context);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0x2, static_cast<int>(context.cpu->ProgramCounter));

			Assert::AreEqual(0x1, static_cast<int>(context.serial_data[0]));
		}

		TEST_METHOD(LoadFF00_IncreaseCyclesBy16_WriteToHighRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x1] = 0x1;

			context.serial_data[0] = 0x1;

			// Act
			Op::LoadFF00(&context);

			// Assert
			Assert::AreEqual(12, context.cycles);
			Assert::AreEqual(0x2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x1, static_cast<int>(result));
		}
	};
}
