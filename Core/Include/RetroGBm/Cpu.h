#pragma once

#include <map>
#include <string>
#include <fstream>

struct EmulatorContext;
class Cartridge;

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
	VBlank = 1,
	STAT = 2,
	Timer = 4,
	Serial = 8,
	Joypad = 16,
};

inline InterruptFlag operator&(InterruptFlag a, InterruptFlag b)
{
	return static_cast<InterruptFlag>(static_cast<int>(a) & static_cast<int>(b));
}

inline InterruptFlag operator&(int a, InterruptFlag b)
{
	return static_cast<InterruptFlag>(a & static_cast<int>(b));
}

class Cpu
{
	Cartridge* m_Cartridge = nullptr;

public:
	Cpu();
	Cpu(Cartridge* cartridge);
	virtual ~Cpu() = default;

	void Init();

	uint16_t ProgramCounter = 0;
	uint16_t StackPointer = 0;

	void Execute(EmulatorContext* context, const uint8_t opcode);

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
	uint8_t GetInterruptFlags() const;

	void SetInterruptEnable(uint8_t data);
	uint8_t GetInterruptEnable() const;

	void HandleInterrupts();

	// Build debug string
	std::string Details();

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

private:
	std::map<RegisterType8, uint8_t> m_Registers;

public:
	bool m_EnablingInterrupts = false;
	bool m_InterruptMasterFlag = false;

	uint8_t m_InterruptFlags;
	uint8_t m_InterruptEnable;

	bool InterruptCheck(InterruptFlag flag);
	void InterruptHandle(InterruptFlag flag, uint16_t address);
};

std::string RegisterTypeString16(RegisterType16 type);

std::string RegisterTypeString8(RegisterType8 type);

std::string FlagString(CpuFlag flag);