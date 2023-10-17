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
	};
}
