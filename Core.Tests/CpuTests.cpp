#include "CppUnitTest.h"

#include <RetroGBm/Cpu.h>
#include <RetroGBm/Cartridge.h>

#include <vector>
#include <tuple>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(CpuTests)
	{
	public:
		TEST_METHOD(SetRegister_RegA_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_A, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_A);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegF_ValueIs100_LowerNibbleOfByteNotSet)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_F, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_F);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(96));
		}

		TEST_METHOD(SetRegister_RegB_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_B, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_B);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegC_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_C, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_C);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegD_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_D, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_D);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegE_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_E, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_E);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegH_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_H, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_H);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegL_ValueIs100)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetRegister(RegisterType8::REG_L, 100);
			uint8_t result = _cpu.GetRegister(RegisterType8::REG_L);

			// Assert
			Assert::AreEqual(result, static_cast<uint8_t>(100));
		}

		TEST_METHOD(SetRegister_RegBC_SetRegisterB_SRegisterC)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			uint16_t data = 12000;
			uint8_t high_data = data >> 8;
			uint8_t low_data = data & 0xFF;

			// Act
			_cpu.SetRegister(RegisterType16::REG_BC, data);

			// Assert
			uint16_t reg_BC = _cpu.GetRegister(RegisterType16::REG_BC);
			Assert::AreEqual(reg_BC, static_cast<uint16_t>(data));

			uint8_t reg_B = _cpu.GetRegister(RegisterType8::REG_B);
			Assert::AreEqual(reg_B, static_cast<uint8_t>(high_data));

			uint8_t reg_C = _cpu.GetRegister(RegisterType8::REG_C);
			Assert::AreEqual(reg_C, static_cast<uint8_t>(low_data));
		}

		TEST_METHOD(SetRegister_RegDE_SetRegisterD_SRegisterE)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			uint16_t data = 12000;
			uint8_t high_data = data >> 8;
			uint8_t low_data = data & 0xFF;

			// Act
			_cpu.SetRegister(RegisterType16::REG_DE, data);

			// Assert
			uint16_t reg_DE = _cpu.GetRegister(RegisterType16::REG_DE);
			Assert::AreEqual(reg_DE, static_cast<uint16_t>(data));

			uint8_t reg_D = _cpu.GetRegister(RegisterType8::REG_D);
			Assert::AreEqual(reg_D, static_cast<uint8_t>(high_data));

			uint8_t reg_E = _cpu.GetRegister(RegisterType8::REG_E);
			Assert::AreEqual(reg_E, static_cast<uint8_t>(low_data));
		}

		TEST_METHOD(SetRegister_RegHL_SetRegisterH_SRegisterL)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			uint16_t data = 12000;
			uint8_t high_data = data >> 8;
			uint8_t low_data = data & 0xFF;

			// Act
			_cpu.SetRegister(RegisterType16::REG_HL, data);

			// Assert
			uint16_t reg_HL = _cpu.GetRegister(RegisterType16::REG_HL);
			Assert::AreEqual(reg_HL, static_cast<uint16_t>(data));

			uint8_t reg_H = _cpu.GetRegister(RegisterType8::REG_H);
			Assert::AreEqual(reg_H, static_cast<uint8_t>(high_data));

			uint8_t reg_L = _cpu.GetRegister(RegisterType8::REG_L);
			Assert::AreEqual(reg_L, static_cast<uint8_t>(low_data));
		}

		TEST_METHOD(SetFlag_FlagZero_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetFlag(CpuFlag::Zero, true);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreNotEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagZero_FlagIsFalse)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetFlag(CpuFlag::Zero, false);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Zero);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(SetFlag_FlagZero_RegisterFHasData_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 255);

			// Act
			_cpu.SetFlag(CpuFlag::Zero, true);

			// Assert
			bool flag = _cpu.GetFlag(CpuFlag::Zero);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagSubtraction_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetFlag(CpuFlag::Subtraction, true);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreNotEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Subtraction);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagSubtraction_FlagIsFalse)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 0x0);

			// Act
			_cpu.SetFlag(CpuFlag::Subtraction, false);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Subtraction);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(SetFlag_FlagSubtraction_RegisterFHasData_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 255);

			// Act
			_cpu.SetFlag(CpuFlag::Subtraction, true);

			// Assert
			bool flag = _cpu.GetFlag(CpuFlag::Subtraction);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagHalfCarry_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetFlag(CpuFlag::HalfCarry, true);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreNotEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::HalfCarry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagHalfCarry_FlagIsFalse)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 0x0);

			// Act
			_cpu.SetFlag(CpuFlag::HalfCarry, false);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::HalfCarry);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(SetFlag_FlagHalfCarry_RegisterFHasData_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 255);

			// Act
			_cpu.SetFlag(CpuFlag::HalfCarry, true);

			// Assert
			bool flag = _cpu.GetFlag(CpuFlag::HalfCarry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagCarry_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);

			// Act
			_cpu.SetFlag(CpuFlag::Carry, true);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreNotEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Carry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(SetFlag_FlagCarry_FlagIsFalse)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 0x0);

			// Act
			_cpu.SetFlag(CpuFlag::Carry, false);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Carry);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(SetFlag_FlagIsSet_SetToFalse)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 0x0);
			_cpu.SetFlag(CpuFlag::Carry, true);

			// Act
			_cpu.SetFlag(CpuFlag::Carry, false);

			// Assert
			uint8_t register_F = _cpu.GetRegister(RegisterType8::REG_F);
			Assert::AreEqual(register_F, static_cast<uint8_t>(0));

			bool flag = _cpu.GetFlag(CpuFlag::Carry);
			Assert::IsFalse(flag);
		}

		TEST_METHOD(SetFlag_FlagCarry_RegisterFHasData_FlagIsTrue)
		{
			// Arrange
			Cartridge cartridge;
			Cpu _cpu(&cartridge);
			_cpu.SetRegister(RegisterType8::REG_F, 255);

			// Act
			_cpu.SetFlag(CpuFlag::Carry, true);

			// Assert
			bool flag = _cpu.GetFlag(CpuFlag::Carry);
			Assert::IsTrue(flag);
		}

		TEST_METHOD(RegisterTypeString16_RegisterHL_StringIsBC)
		{
			// Act
			std::string name = RegisterTypeString16(RegisterType16::REG_BC);

			// Assert
			Assert::AreEqual("BC", name.c_str());
		}

		TEST_METHOD(RegisterTypeString16_RegisterHL_StringIsDE)
		{
			// Act
			std::string name = RegisterTypeString16(RegisterType16::REG_DE);

			// Assert
			Assert::AreEqual("DE", name.c_str());
		}

		TEST_METHOD(RegisterTypeString16_RegisterHL_StringIsHL)
		{
			// Act
			std::string name = RegisterTypeString16(RegisterType16::REG_HL);

			// Assert
			Assert::AreEqual("HL", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterA_StringIsA)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_A);

			// Assert
			Assert::AreEqual("A", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterF_StringIsF)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_F);

			// Assert
			Assert::AreEqual("F", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterB_StringIsB)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_B);

			// Assert
			Assert::AreEqual("B", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterC_StringIsC)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_C);

			// Assert
			Assert::AreEqual("C", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterD_StringIsD)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_D);

			// Assert
			Assert::AreEqual("D", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterE_StringIsE)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_E);

			// Assert
			Assert::AreEqual("E", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterH_StringIsH)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_H);

			// Assert
			Assert::AreEqual("H", name.c_str());
		}

		TEST_METHOD(RegisterTypeString8_RegisterL_StringIsL)
		{
			// Act
			std::string name = RegisterTypeString8(RegisterType8::REG_L);

			// Assert
			Assert::AreEqual("L", name.c_str());
		}

		TEST_METHOD(SetInterrupt_InterruptFlagIsMaskedxE0_ReturnsE1)
		{
			// Arrange
			Cartridge cartridge;
			Cpu cpu(&cartridge);

			// Act
			cpu.SetInterrupt(1);
			uint8_t result = cpu.GetInterruptFlags();

			// Assert
			Assert::AreEqual(0xE1, static_cast<int>(result));
		}
	};
}
