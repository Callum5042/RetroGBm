#include "CppUnitTest.h"
#include "MockCartridge.h"
#include "NullSoundOutput.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Instructions.h>
#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Cartridge/CartridgeROM.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(InstructionsTests)
	{
	public:
		TEST_METHOD(Nop_IncreaseProgramCounter)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->ProgramCounter = 0;

			// Act
			Op::Nop(emulator.GetContext());

			// Assert
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->ProgramCounter));
		}

		TEST_METHOD(LoadR8_LoadRegisterIntoRegister)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_E, 0x0);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_D, 0x5);

			// Act
			Op::LoadR8(emulator.GetContext(), RegisterType8::REG_E, RegisterType8::REG_D);

			// Assert
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_D)));
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_E)));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x5);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x15, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetCarryFlags)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xFF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xA);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x9, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0xA, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetZeroFlags)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xFF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(AddR8_AddRegisterToRegister_Overflows_SetHalfCarryFlags)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0xF);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::AddR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x5, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultIsZero_SetZeroFlag)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x15);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x15, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultWillUnderflow_SetCarryFlag)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x15);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x16);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x16, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(SubR8_SubRegisterToRegister_ResultWillUnderflowNibble_SetHalfCarryFlag)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_A, 0x10);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::SubR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_A)));
			Assert::AreEqual(0x1, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(IncR8_IncrementRegisterBy1_CarryFlagIsIgnored)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x6, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(IncR8_ResultsOverflown_SetZeroFlag_IgnoreCarryFlag)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xFF);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, false);
			emulator.GetCpu()->SetFlag(CpuFlag::Zero, false);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
		}

		TEST_METHOD(IncR8_ResultsOverflown_UnderflownNibble_SetHalfCarry)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0xF);
			emulator.GetCpu()->SetFlag(CpuFlag::HalfCarry, false);

			// Act
			Op::IncR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x10, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_CarryFlagIsIgnored)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x5);
			emulator.GetCpu()->SetFlag(CpuFlag::Carry, true);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x4, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Carry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_ResultIsZero_ZeroFlagIsSet)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x1);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(DecR8_DecrementRegisterBy1_ResultUnderflowsNibble_SetHalfCarryFlag)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_B, 0x10);

			// Act
			Op::DecR8(emulator.GetContext(), RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(0xF, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_B)));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::Subtraction));
			Assert::IsFalse(emulator.GetCpu()->GetFlag(CpuFlag::Zero));
			Assert::IsTrue(emulator.GetCpu()->GetFlag(CpuFlag::HalfCarry));
		}

		TEST_METHOD(LoadN8_SetRegisterToBusValue)
		{
			// Arrange
			CartridgeDataV2 cartridge_data;
			cartridge_data.data.resize(0x3FFF);
			cartridge_data.data[0x33] = 0xA;

			NullSoundOutput sound_output;
			Emulator emulator(std::make_unique<CartridgeROM>(cartridge_data), &sound_output);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_D, 0x0);
			emulator.GetCpu()->ProgramCounter = 0x32;

			// Act
			Op::LoadN8(emulator.GetContext(), RegisterType8::REG_D);

			// Assert
			Assert::AreEqual(0xA, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType8::REG_D)));
		}

		TEST_METHOD(StoreN8_SetValuePointerToAtBusToBusValue)
		{
			// Arrange
			CartridgeDataV2 cartridge_data;
			cartridge_data.data.resize(0x3FFF);
			cartridge_data.data[0x33] = 0xA;

			NullSoundOutput sound_output;
			Emulator emulator(std::make_unique<CartridgeROM>(cartridge_data), &sound_output);
			emulator.GetCpu()->SetRegister(RegisterType16::REG_HL, 0xC000);
			emulator.GetCpu()->ProgramCounter = 0x32;

			emulator.WriteBus(0xC000, 0x0);

			// Act
			Op::StoreN8(emulator.GetContext(), RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(0xA, static_cast<int>(emulator.ReadBus(0xC000)));
		}

		TEST_METHOD(StoreR8_SetValuePointerToAtBusToBusValue)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->SetRegister(RegisterType16::REG_HL, 0xC000);
			emulator.GetCpu()->SetRegister(RegisterType8::REG_D, 0xA);
			emulator.GetCpu()->ProgramCounter = 0x32;
			emulator.WriteBus(0xC000, 0x0);

			// Act
			Op::StoreR8(emulator.GetContext(), RegisterType8::REG_D, RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual(0xA, static_cast<int>(emulator.ReadBus(0xC000)));
		}

		TEST_METHOD(Pop16_Pushed16_ShouldPop16_BeSameValue)
		{
			// Arrange
			NullSoundOutput sound_output;
			Emulator emulator(&sound_output);
			emulator.GetCpu()->ProgramCounter = 0x32;

			emulator.GetCpu()->SetRegister(RegisterType16::REG_BC, 0xAABB);
			Op::PushR16(emulator.GetContext(), RegisterType16::REG_BC);
			emulator.GetCpu()->SetRegister(RegisterType16::REG_BC, 0x0);

			// Act
			Op::PopR16(emulator.GetContext(), RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual(0xAABB, static_cast<int>(emulator.GetCpu()->GetRegister(RegisterType16::REG_BC)));
		}

		TEST_METHOD(ReturnInterrupt_EnableMasterInterruptsImmediately)
		{
			// Arrange
			CartridgeDataV2 cartridge_data;
			cartridge_data.data.resize(0x3FFF);
			cartridge_data.data[0x0] = 0xAA;
			cartridge_data.data[0x1] = 0xBB;

			NullSoundOutput sound_output;
			Emulator emulator(std::make_unique<CartridgeROM>(cartridge_data), &sound_output);
			emulator.GetCpu()->ProgramCounter = 0x32;
			emulator.GetCpu()->StackPointer = 0x0;
			emulator.GetCpu()->m_InterruptMasterFlag = false;

			// Act
			Op::ReturnInterrupt(emulator.GetContext());

			// Assert
			Assert::IsTrue(emulator.GetCpu()->m_InterruptMasterFlag);
			Assert::AreEqual(0xBBAA, static_cast<int>(emulator.GetCpu()->ProgramCounter));
		}
	};
}
