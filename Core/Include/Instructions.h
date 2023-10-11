#pragma once

#include <cstdint>
#include <string>
#include "Cartridge.h"
#include "Cpu.h"

struct EmulatorContext;

namespace Op
{
	std::string Nop(EmulatorContext* context);

	std::string JumpN16(EmulatorContext* context);

	std::string JumpFlagN16(EmulatorContext* context, CpuFlag flag, bool condition);

	std::string JumpHL(EmulatorContext* context);

	std::string JumpRelativeN8(EmulatorContext* context);

	std::string XorR8(EmulatorContext* context, RegisterType8 type);

	std::string XorN8(EmulatorContext* context);

	std::string XorR16(EmulatorContext* context, RegisterType16 type);

	std::string LoadN16(EmulatorContext* context, RegisterType16 type);

	std::string LoadR8(EmulatorContext* context, RegisterType8 reg1, RegisterType8 reg2);

	std::string LoadN8(EmulatorContext* context, RegisterType8 type);

	std::string LoadIncrementHL(EmulatorContext* context);

	std::string LoadDecrementHL(EmulatorContext* context);

	std::string StoreR8(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	std::string StoreN8(EmulatorContext* context, RegisterType16 reg_pointer);

	std::string LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	std::string StoreIndirectAC(EmulatorContext* context);

	std::string LoadIndirectAC(EmulatorContext* context);

	std::string StoreIndirectA16(EmulatorContext* context);

	std::string LoadIndirectA16(EmulatorContext* context);

	std::string AddR8(EmulatorContext* context, RegisterType8 reg);

	std::string AddN8(EmulatorContext* context);

	std::string AddIndirectHL(EmulatorContext* context);

	std::string AddSP(EmulatorContext* context);

	std::string AddR16(EmulatorContext* context, RegisterType16 reg);
}