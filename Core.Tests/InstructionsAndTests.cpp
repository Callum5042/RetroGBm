#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsAndTests)
	{
	public:
		TEST_METHOD(AndN8_ResultIsZero_SetZeroFlag_SetRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cpu = std::make_unique<Cpu>();
			context.cycles = 0;
			context.cartridge = std::make_unique<CartridgeInfo>();
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x10);

			context.cpu->SetRegister(RegisterType8::REG_A, 0x1);

			// Act
			Op::AndN8(&context);

			// Assert
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(8, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AndN8_ResultIsNotZero_DoNotSetZeroFlag_SetRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cpu = std::make_unique<Cpu>();
			context.cycles = 0;
			context.cartridge = std::make_unique<CartridgeInfo>();
			context.cartridge->data.push_back(0x0);
			context.cartridge->data.push_back(0x3);

			context.cpu->SetRegister(RegisterType8::REG_A, 0x1);

			// Act
			Op::AndN8(&context);

			// Assert
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(8, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x1, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}
