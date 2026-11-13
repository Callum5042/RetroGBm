#include "CppUnitTest.h"
#include "NullDisplayOutput.h"
#include "NullNetworkOutput.h"
#include "NullSoundOutput.h"

#include <RetroGBm/Cpu.h>
#include <RetroGBm/Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	// Interrupt flag bits, lowest bit = highest priority
	constexpr uint8_t VBlank = 0x01; // vector $40
	constexpr uint8_t Stat = 0x02;   // vector $48
	constexpr uint8_t Timer = 0x04;  // vector $50

	TEST_CLASS(InterruptTests)
	{
	public:
		TEST_METHOD(HandleInterrupts_TwoPending_ServicesHighestPriority)
		{
			// Arrange: a normal stack in WRAM, VBlank and STAT both pending
			NullDisplayOutput display;
			NullSoundOutput sound;
			NullNetworkOutput network;
			Emulator emulator(&display, &sound, &network);
			Cpu* cpu = emulator.GetCpu();

			cpu->ProgramCounter = 0x1234;
			cpu->StackPointer = 0xD000;
			cpu->m_InterruptMasterFlag = true;
			cpu->SetInterruptEnable(VBlank | Stat);
			cpu->SetInterrupt(VBlank | Stat);

			// Act
			cpu->HandleInterrupts();

			// Assert: VBlank wins, STAT is left pending for later
			Assert::AreEqual(0x40, static_cast<int>(cpu->ProgramCounter));
			Assert::AreEqual(Stat | 0xE0, static_cast<int>(cpu->GetInterruptFlags()));
			Assert::IsFalse(cpu->GetInterruptMasterFlag());

			// The return address was pushed high byte first
			Assert::AreEqual(0xCFFE, static_cast<int>(cpu->StackPointer));
			Assert::AreEqual(0x12, static_cast<int>(emulator.ReadBus(0xCFFF)));
			Assert::AreEqual(0x34, static_cast<int>(emulator.ReadBus(0xCFFE)));
		}

		TEST_METHOD(HandleInterrupts_StackAt0000_HighBytePushLandsOnIE)
		{
			// Arrange: SP is $0000, so the first push decrements it to $FFFF, which is IE
			NullDisplayOutput display;
			NullSoundOutput sound;
			NullNetworkOutput network;
			Emulator emulator(&display, &sound, &network);
			Cpu* cpu = emulator.GetCpu();

			cpu->ProgramCounter = 0x1F34;
			cpu->StackPointer = 0x0000;
			cpu->m_InterruptMasterFlag = true;
			cpu->SetInterruptEnable(VBlank);
			cpu->SetInterrupt(VBlank);

			// Act
			cpu->HandleInterrupts();

			// Assert: the high byte of PC ($1F) overwrote IE, the low byte ($34) went to HRAM at $FFFE
			Assert::AreEqual(0x1F, static_cast<int>(cpu->GetInterruptEnable()));
			Assert::AreEqual(0x34, static_cast<int>(emulator.ReadBus(0xFFFE)));
		}

		TEST_METHOD(HandleInterrupts_IEPushDisablesOnlyPendingInterrupt_DispatchCancelled)
		{
			// Arrange: only Timer is pending. PC is $0200, so the high byte push writes $02 to IE,
			// which enables STAT only and switches Timer off. Same as round 1 of ie_push.
			NullDisplayOutput display;
			NullSoundOutput sound;
			NullNetworkOutput network;
			Emulator emulator(&display, &sound, &network);
			Cpu* cpu = emulator.GetCpu();

			cpu->ProgramCounter = 0x0200;
			cpu->StackPointer = 0x0000;
			cpu->m_InterruptMasterFlag = true;
			cpu->SetInterruptEnable(Timer);
			cpu->SetInterrupt(Timer);

			// Act
			cpu->HandleInterrupts();

			// Assert: nothing is left to service, so the CPU jumps to $0000 instead of $50
			Assert::AreEqual(0x0000, static_cast<int>(cpu->ProgramCounter));

			// Timer was never serviced, so its IF bit is still set
			Assert::AreEqual(Timer | 0xE0, static_cast<int>(cpu->GetInterruptFlags()));

			// IME is still switched off, as with a normal dispatch
			Assert::IsFalse(cpu->GetInterruptMasterFlag());
		}

		TEST_METHOD(HandleInterrupts_IEPushDisablesChosenInterrupt_ServicesNextOneInstead)
		{
			// Arrange: VBlank and STAT are pending, so VBlank would normally win. The high byte
			// push writes $02 to IE, which switches VBlank off but keeps STAT. Same as round 4 of ie_push.
			NullDisplayOutput display;
			NullSoundOutput sound;
			NullNetworkOutput network;
			Emulator emulator(&display, &sound, &network);
			Cpu* cpu = emulator.GetCpu();

			cpu->ProgramCounter = 0x0200;
			cpu->StackPointer = 0x0000;
			cpu->m_InterruptMasterFlag = true;
			cpu->SetInterruptEnable(VBlank | Stat);
			cpu->SetInterrupt(VBlank | Stat);

			// Act
			cpu->HandleInterrupts();

			// Assert: STAT is serviced instead of VBlank
			Assert::AreEqual(0x48, static_cast<int>(cpu->ProgramCounter));

			// STAT's IF bit is cleared, VBlank's is still set because it was skipped
			Assert::AreEqual(VBlank | 0xE0, static_cast<int>(cpu->GetInterruptFlags()));
		}
	};
}
