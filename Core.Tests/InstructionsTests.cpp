#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsTests)
	{
	public:

		TEST_METHOD(Nop_IncreaseCyclesBy4)
		{
			// Arrange
			EmulatorContext context;
			context.cpu = std::make_unique<Cpu>();
			context.cycles = 0;

			// Act
			Op::Nop(&context);

			// Assert
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));
			Assert::AreEqual(4, context.cycles);
		}

		TEST_METHOD(EnableInterrupts_IncreaseCyclesBy4_SetInterruptMasterFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cpu->DisableMasterInterrupts();

			// Act
			Op::EnableInterrupts(&context);

			// Assert
			Assert::AreEqual(0x4, context.cycles);

			bool result = context.cpu->GetInterruptMasterFlag();
			Assert::IsTrue(result);
		}

		TEST_METHOD(DisableInterrupts_IncreaseCyclesBy4_SetInterruptMasterFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();
			context.cpu->EnableMasterInterrupts();

			// Act
			Op::DisableInterrupts(&context);

			// Assert
			Assert::AreEqual(0x4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			bool result = context.cpu->GetInterruptMasterFlag();
			Assert::IsFalse(result);
		}

		TEST_METHOD(ComplementA_IncreaseCyclesBy4_FlipBits)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0xF);
			context.cpu->SetFlag(CpuFlag::Subtraction, false);
			context.cpu->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::ComplementA(&context);

			// Assert
			Assert::AreEqual(0x4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0xF0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::HalfCarry));
		}
	};
}
