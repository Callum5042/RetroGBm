#pragma once

#include <map>
#include <string>

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
	REG_AF,
	REG_BC,
	REG_DE,
	REG_HL,
	REG_SP
};

enum class CpuFlag
{
	Zero,
	Subtraction,
	HalfCarry,
	Carry
};

enum class InterruptFlag
{
	VBlank,
	STAT,
	Timer,
	Serial,
	Joypad
};

class Cpu
{
public:
	Cpu();
	virtual ~Cpu() = default;

	uint16_t ProgramCounter = 0;
	uint16_t StackPointer = 0;

	// Registers
	void SetRegister(RegisterType8 type, uint8_t data);
	void SetRegister(RegisterType16 type, uint16_t data);
	void SetRegister(RegisterType16 type, uint8_t data_high, uint8_t data_low);

	uint8_t GetRegister(RegisterType8 type) const;
	uint16_t GetRegister(RegisterType16 type) const;

	// Flags
	void SetFlag(CpuFlag flag, bool data);
	bool GetFlag(CpuFlag flag) const;

	// Interrupts
	void EnableMasterInterrupts();
	void DisableMasterInterrupts();
	bool GetInterruptMasterFlag() const;

	void RequestInterrupt(InterruptFlag flag);
	void SetInterrupt(uint8_t data);

	void InterruptEnable(uint8_t data);
	uint8_t GetInterruptEnable() const;

	// Build debug string
	std::string Details();

private:
	std::map<RegisterType8, uint8_t> m_Registers;

	bool m_InterruptMasterFlag = false;

	// TODO: Not actually sure the difference between these 2
	std::map<InterruptFlag, bool> m_Interrupts;
	uint8_t m_InterruptFlags;

	uint8_t m_InterruptEnable;
};

std::string RegisterTypeString16(RegisterType16 type);

std::string RegisterTypeString8(RegisterType8 type);

std::string FlagString(CpuFlag flag);