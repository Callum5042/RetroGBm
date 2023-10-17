#include "CppUnitTest.h"
#include <Instructions.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InstructionsTests
{
	TEST_CLASS(InstructionsShiftTests)
	{
	public:
		TEST_METHOD(RotateRegisterA_ShiftRight_CarryFlagNotSet)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x8);
			context.cpu->SetFlag(CpuFlag::Carry, false);

			// Act
			Op::RotateRegisterA(&context);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x4, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(RotateRegisterA_WillUnderFlow_ShiftRight_CarryFlagSet)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x1);
			context.cpu->SetFlag(CpuFlag::Carry, false);

			// Act
			Op::RotateRegisterA(&context);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x0, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(context.cpu->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(RotateRegisterA_CarryFlagSetShiftRight_CarryFlagNotSet)
		{
			// Arrange
			EmulatorContext context;
			context.cycles = 0;
			context.cpu = std::make_unique<Cpu>();

			context.cpu->SetRegister(RegisterType8::REG_A, 0x8);
			context.cpu->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::RotateRegisterA(&context);

			// Assert
			Assert::AreEqual(4, context.cycles);
			Assert::AreEqual(1, static_cast<int>(context.cpu->ProgramCounter));

			uint8_t result = context.cpu->GetRegister(RegisterType8::REG_A);
			Assert::AreEqual(0x84, static_cast<int>(result));

			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::HalfCarry));
			Assert::IsFalse(context.cpu->GetFlag(CpuFlag::Carry));
		}
	};
}