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

		TEST_METHOD(CallN16_IncreaseCyclesBy24)
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
	};
}