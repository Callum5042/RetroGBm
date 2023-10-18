#pragma once

#include <cstdint>
#include <string>
#include "Cartridge.h"
#include "Cpu.h"

struct EmulatorContext;

namespace Op
{
	std::string Nop(EmulatorContext* context);

	std::string Stop(EmulatorContext* context);

	std::string EnableInterrupts(EmulatorContext* context);

	std::string DisableInterrupts(EmulatorContext* context);

	std::string JumpN16(EmulatorContext* context);

	std::string JumpFlagN16(EmulatorContext* context, CpuFlag flag, bool condition);

	std::string JumpHL(EmulatorContext* context);

	std::string JumpRelativeN8(EmulatorContext* context);

	std::string JumpRelativeFlagN8(EmulatorContext* context, CpuFlag flag, bool condition);

	std::string JumpRelativeFlagNotSet(EmulatorContext* context, CpuFlag flag);

	std::string JumpRelativeFlagSet(EmulatorContext* context, CpuFlag flag);

	std::string XorR8(EmulatorContext* context, RegisterType8 type);

	std::string XorN8(EmulatorContext* context);

	std::string XorR16(EmulatorContext* context, RegisterType16 type);

	std::string LoadN16(EmulatorContext* context, RegisterType16 type);

	std::string LoadR8(EmulatorContext* context, RegisterType8 reg1, RegisterType8 reg2);

	std::string LoadN8(EmulatorContext* context, RegisterType8 type);

	std::string LoadIndirectSP(EmulatorContext* context);

	std::string LoadHLFromSP(EmulatorContext* context);

	std::string LoadHLFromSPRelative(EmulatorContext* context);

	std::string StoreIncrementHL(EmulatorContext* context);

	std::string StoreDecrementHL(EmulatorContext* context);

	std::string LoadIncrementHL(EmulatorContext* context);

	std::string LoadDecrementHL(EmulatorContext* context);

	std::string StoreIndirectR8(EmulatorContext* context, RegisterType8 reg);

	std::string LoadIndirectR8(EmulatorContext* context, RegisterType8 reg);

	std::string StoreFF00(EmulatorContext* context);

	std::string LoadFF00(EmulatorContext* context);

	std::string StoreR8(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	std::string StoreN8(EmulatorContext* context, RegisterType16 reg_pointer);

	std::string LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	std::string StoreIndirectAC(EmulatorContext* context);

	std::string LoadIndirectAC(EmulatorContext* context);

	std::string StoreIndirectA16(EmulatorContext* context);

	std::string LoadIndirectA16(EmulatorContext* context);

	std::string AddR8(EmulatorContext* context, RegisterType8 reg);

	std::string AddN8(EmulatorContext* context);

	std::string SubN8(EmulatorContext* context);

	std::string AddIndirectHL(EmulatorContext* context);

	std::string AddSP(EmulatorContext* context);

	std::string AddR16(EmulatorContext* context, RegisterType16 reg);

	std::string IncR8(EmulatorContext* context, RegisterType8 reg);

	std::string DecR8(EmulatorContext* context, RegisterType8 reg);

	std::string CallN16(EmulatorContext* context);

	std::string CallN16Condition(EmulatorContext* context, CpuFlag flag, bool condition);

	std::string CallN16FlagNotSet(EmulatorContext* context, CpuFlag flag);

	std::string Return(EmulatorContext* context);

	std::string ReturnCondition(EmulatorContext* context, CpuFlag flag, bool condition);

	std::string ReturnFlagNotSet(EmulatorContext* context, CpuFlag flag);

	std::string ReturnFlagSet(EmulatorContext* context, CpuFlag flag);

	std::string CompareR8(EmulatorContext* context, RegisterType8 reg);

	std::string CompareN8(EmulatorContext* context);

	std::string CompareIndirectHL(EmulatorContext* context);

	std::string PushR16(EmulatorContext* context, RegisterType16 reg);

	std::string PopR16(EmulatorContext* context, RegisterType16 reg);

	std::string IncR16(EmulatorContext* context, RegisterType16 reg);

	std::string DecR16(EmulatorContext* context, RegisterType16 reg);

	std::string DecIndirectHL(EmulatorContext* context);

	std::string OrR8(EmulatorContext* context, RegisterType8 reg);

	std::string OrN8(EmulatorContext* context);

	std::string OrHL(EmulatorContext* context);

	std::string AndR8(EmulatorContext* context, RegisterType8 reg);

	std::string AndN8(EmulatorContext* context);

	std::string ExtendedPrefix(EmulatorContext* context);

	std::string RotateRegisterA(EmulatorContext* context);

	std::string AddCarryN8(EmulatorContext* context);

	std::string SubCarryN8(EmulatorContext* context);

	std::string Daa(EmulatorContext* context);

	std::string ComplementA(EmulatorContext* context);

	std::string SetCarryFlag(EmulatorContext* context);
}