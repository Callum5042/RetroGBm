#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsDecTests)
	{
	public:
		TEST_METHOD(DecR8_IncreaseCyclesBy4_DecreaseRegister)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0xD);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::Zero, true);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0xC, static_cast<int>(result));

			bool subtract_flag = context.cpu->GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(subtract_flag);

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsFalse(subtract_flag);
		}
		TEST_METHOD(DecR8_RegIs01_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_B, 0x01);
			context.cpu->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::DecR8(&context, RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(4, context.cycles);

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(0x0, static_cast<int>(result));

			bool zero_flag = context.cpu->GetFlag(CpuFlag::Zero);
			Assert::IsTrue(zero_flag);
		}
	};
}