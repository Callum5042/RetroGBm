#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsTests)
	{
	public:

		TEST_METHOD(Nop_IncreaseCyclesBy4)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;

			// Act
			Op::Nop(&context);

			// Assert
			Assert::AreEqual(4, context.cycles);
		}

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

		TEST_METHOD(XorR8_RegARegA_IncreaseCyclesBy4_SetRegA_FlagZeroTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			// Act
			Op::XorR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(XorR8_RegBRegA_IncreaseCyclesBy4_SetRegA_FlagZeroFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x20);

			// Act
			Op::XorR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x30, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(XorR8_RegBRegA_IncreaseCyclesBy4_SetRegA_FlagZeroTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x10);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::XorR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(XorN8_IncreaseCyclesBy8_IncreaseProgramCounterBy1_SetRegA_ZeroFlagTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cartridge->data.push_back(0x20);

			// Act
			Op::XorN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(XorN8_IncreaseCyclesBy8_IncreaseProgramCounterBy1_SetRegA_ZeroFlagFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cartridge->data.push_back(0x30);

			// Act
			Op::XorN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x10, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(XorR16_IncreaseCyclesBy8_ZeroFlagFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0xA);

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0xA] = 0x30;

			// Act
			Op::XorR16(&context, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x10, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(XorR16_IncreaseCyclesBy8_ZeroFlagTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cpu->SetRegister(RegisterType16::REG_HL, 0xA);

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0xA] = 0x20;

			// Act
			Op::XorR16(&context, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(8, context.cycles);

			uint8_t reg = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(reg));

			bool flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}
	};
}
