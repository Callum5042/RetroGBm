#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsCompareTests)
	{
	public:
		TEST_METHOD(CompareR8_RegisterBNotEqualA_ZeroFlagFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cpu->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::CompareR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(CompareR8_RegisterIsA_SetFlags)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			
			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);

			// Act
			Op::CompareR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
		
		TEST_METHOD(CompareN8_IncreaseCyclesBy8_ResultDoNotEqual_ZeroFlagIsFalse)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x10);

			// Act
			Op::CompareN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
		
		TEST_METHOD(CompareN8_IncreaseCyclesBy8_ResultDoEqual_ZeroFlagIsTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x20);

			// Act
			Op::CompareN8(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
		
		TEST_METHOD(CompareIndirectHL_IncreaseCyclesBy8_ResultDoEqual_ZeroFlagIsTrue)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x20;

			context.cpu->SetRegister(RegisterType16::REG_HL, 0x5);
			context.cpu->SetRegister(RegisterType8::REG_A, 0x20);

			// Act
			Op::CompareIndirectHL(&context);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}
