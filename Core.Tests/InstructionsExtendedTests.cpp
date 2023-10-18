#include "CppUnitTest.h"
#include <Instructions.h>
#include <ExtendedInstructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(ExtendedInstructionsTests)
	{
	public:
		TEST_METHOD(SwapR8_SwapNibblesInRegister_UnsetFlags)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x83);

			context.cpu->SetFlag(CpuFlag::Zero, true);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::HalfCarry, true);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			CB::SwapR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x38, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SwapR8_ByteIs0_SetZeroFlag)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x0);

			context.cpu->SetFlag(CpuFlag::Zero, false);
			context.cpu->SetFlag(CpuFlag::Subtraction, true);
			context.cpu->SetFlag(CpuFlag::HalfCarry, true);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			CB::SwapR8(&context, RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(8, context.cycles);
			Assert::AreEqual(2, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}