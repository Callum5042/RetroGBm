#pragma once

#include <map>

enum class RegisterType8
{
	REG_A,
	REG_F,
	REG_B,
	REG_C,
	REG_D,
	REG_E,
	REG_H,
	REG_L
};

enum class RegisterType16
{
	REG_BC,
	REG_DE,
	REG_HL
};

enum class CpuFlag
{
	Zero,
	Subtraction,
	HalfCarry,
	Carry
};

class Cpu
{
public:
	Cpu() = default;
	virtual ~Cpu() = default;

	// Registers
	void SetRegister(RegisterType8 type, uint8_t data);
	void SetRegister(RegisterType16 type, uint16_t data);
	void SetRegister(RegisterType16 type, uint8_t data_high, uint8_t data_low);

	uint8_t GetRegister(RegisterType8 type) const;
	uint16_t GetRegister(RegisterType16 type) const;

	// Flags
	void SetFlag(CpuFlag flag, bool data);
	bool GetFlag(CpuFlag flag) const;

private:
	std::map<RegisterType8, uint8_t> m_Registers;
};