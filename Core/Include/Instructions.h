#pragma once

#include "Cpu.h"

struct EmulatorContext;

namespace Op
{
	void Nop(EmulatorContext* context);

	void Stop(EmulatorContext* context);

	void EnableInterrupts(EmulatorContext* context);

	void DisableInterrupts(EmulatorContext* context);

	void JumpN16(EmulatorContext* context);

	void JumpFlagN16(EmulatorContext* context, CpuFlag flag, bool condition);

	void JumpHL(EmulatorContext* context);

	void JumpRelativeN8(EmulatorContext* context);

	void JumpRelativeFlagNotSet(EmulatorContext* context, CpuFlag flag);

	void JumpRelativeFlagSet(EmulatorContext* context, CpuFlag flag);

	void XorR8(EmulatorContext* context, RegisterType8 type);

	void XorN8(EmulatorContext* context);

	void XorR16(EmulatorContext* context, RegisterType16 type);

	void LoadN16(EmulatorContext* context, RegisterType16 type);

	void LoadR8(EmulatorContext* context, RegisterType8 reg1, RegisterType8 reg2);

	void LoadN8(EmulatorContext* context, RegisterType8 type);

	void LoadIndirectSP(EmulatorContext* context);

	void LoadHLFromSP(EmulatorContext* context);

	void LoadHLFromSPRelative(EmulatorContext* context);

	void StoreIncrementHL(EmulatorContext* context);

	void StoreDecrementHL(EmulatorContext* context);

	void LoadIncrementHL(EmulatorContext* context);

	void LoadDecrementHL(EmulatorContext* context);

	void StoreIndirectR8(EmulatorContext* context, RegisterType8 reg);

	void LoadIndirectR8(EmulatorContext* context, RegisterType8 reg);

	void StoreIO(EmulatorContext* context);

	void LoadIO(EmulatorContext* context);

	void Halt(EmulatorContext* context);

	void StoreR8(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	void StoreN8(EmulatorContext* context, RegisterType16 reg_pointer);

	void LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer);

	void StoreIndirectAC(EmulatorContext* context);

	void LoadIndirectAC(EmulatorContext* context);

	void AddR8(EmulatorContext* context, RegisterType8 reg);

	void AddN8(EmulatorContext* context);

	void SubR8(EmulatorContext* context, RegisterType8 reg);

	void SubIndirectHL(EmulatorContext* context);

	void SubN8(EmulatorContext* context);

	void AddIndirectHL(EmulatorContext* context);

	void AddSP(EmulatorContext* context);

	void AddR16(EmulatorContext* context, RegisterType16 reg);

	void IncR8(EmulatorContext* context, RegisterType8 reg);

	void DecR8(EmulatorContext* context, RegisterType8 reg);

	void CallN16(EmulatorContext* context);

	void CallN16FlagNotSet(EmulatorContext* context, CpuFlag flag);

	void CallN16FlagSet(EmulatorContext* context, CpuFlag flag);

	void Return(EmulatorContext* context);

	void ReturnInterrupt(EmulatorContext* context);

	void ReturnFlagNotSet(EmulatorContext* context, CpuFlag flag);

	void ReturnFlagSet(EmulatorContext* context, CpuFlag flag);

	void CompareR8(EmulatorContext* context, RegisterType8 reg);

	void CompareN8(EmulatorContext* context);

	void CompareIndirectHL(EmulatorContext* context);

	void PushR16(EmulatorContext* context, RegisterType16 reg);

	void PopR16(EmulatorContext* context, RegisterType16 reg);

	void IncR16(EmulatorContext* context, RegisterType16 reg);

	void DecR16(EmulatorContext* context, RegisterType16 reg);

	void DecIndirectHL(EmulatorContext* context);

	void IncIndirectHL(EmulatorContext* context);

	void OrR8(EmulatorContext* context, RegisterType8 reg);

	void OrN8(EmulatorContext* context);

	void OrHL(EmulatorContext* context);

	void AndR8(EmulatorContext* context, RegisterType8 reg);

	void AndIndirectHL(EmulatorContext* context);

	void AndN8(EmulatorContext* context);

	void ExtendedPrefix(EmulatorContext* context);

	void RotateRegisterRightA(EmulatorContext* context);

	void RotateRegisterLeftA(EmulatorContext* context);

	void AddCarryN8(EmulatorContext* context);

	void SubCarryN8(EmulatorContext* context);

	void Daa(EmulatorContext* context);

	void ComplementA(EmulatorContext* context);

	void SetCarryFlag(EmulatorContext* context);

	void Rst(EmulatorContext* context, uint8_t offset);

	void ComplementCarryFlag(EmulatorContext* context);

	void AddCarryR8(EmulatorContext* context, RegisterType8 reg);

	void AddCarryIndirectHL(EmulatorContext* context);

	void SubCarryR8(EmulatorContext* context, RegisterType8 reg);

	void SubCarryIndirectHL(EmulatorContext* context);

	void RotateRegisterLeftCarryA(EmulatorContext* context);

	void RotateRegisterRightCarryA(EmulatorContext* context);
}