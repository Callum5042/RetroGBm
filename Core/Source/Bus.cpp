#include "Bus.h"
#include <exception>

// 0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
// 4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper(if any)
// 8000	9FFF	8 KiB Video RAM(VRAM)	In CGB mode, switchable bank 0 / 1
// A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
// C000	CFFF	4 KiB Work RAM(WRAM)
// D000	DFFF	4 KiB Work RAM(WRAM)	In CGB mode, switchable bank 1~7
// E000	FDFF	Mirror of C000~DDFF(ECHO RAM)	Nintendo says use of this area is prohibited.
// FE00	FE9F	Object attribute memory(OAM)
// FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited
// FF00	FF7F	I / O Registers
// FF80	FFFE	High RAM(HRAM)
// FFFF	FFFF	Interrupt Enable register (IE)

const uint8_t ReadFromBus(const CartridgeInfo* cartridge, const uint16_t address)
{
	if (address < 0x8000)
	{
		// Read from ROM
		return cartridge->data[address];  
	}
	else if (address < 0xA000)
	{
		// VRAM (video RAM)
		throw std::exception("Not implemented 'ReadFromBus' VRAM");
	}
	else if (address < 0xC000)
	{
		// Read from RAM
		return cartridge->data[address];
	}
	else if (address < 0x0E000)
	{
		// WRAM (working RAM)
		throw std::exception("Not implemented 'ReadFromBus' WRAM");
	}
	else if (address < 0xFE00)
	{
		// reserved echo RAM
		return 0;
	}
	else if (address < 0xFEA0)
	{
		// OAM
		throw std::exception("Not implemented 'ReadFromBus' OAM");
	}
	else if (address < 0xFF00)
	{
		// Reserved unused
		return 0;
	}
	else if (address < 0xFF80)
	{
		// I/O registers
		throw std::exception("Not implemented 'ReadFromBus' I/O registers");
	}
	else if (address < 0xFF80)
	{
		// HRAM (high RAM)
		throw std::exception("Not implemented 'ReadFromBus' HRAM");
	}
	else if (address == 0xFFFF)
	{
		// Interrupt Enable register (IE)
		throw std::exception("Not implemented 'ReadFromBus' Interrupt Enable register (IE)");
	}

	throw std::exception(std::format("Not implemented 'ReadFromBus' 0x{:x}", address).c_str());
}

void WriteToBus(CartridgeInfo* cartridge, uint16_t address, uint8_t data)
{
	if (address < 0x8000)
	{
		// Write to ROM
		cartridge->data[address] = data;
		return;
	}
	else if (address < 0xA000)
	{
		// VRAM (video RAM)
		throw std::exception("Not implemented 'WriteToBus' VRAM");
	}
	else if (address < 0xC000)
	{
		// Write to RAM
		cartridge->data[address] = data;
		return;
	}
	else if (address < 0x0E000)
	{
		// WRAM (working RAM)
		throw std::exception("Not implemented 'WriteToBus' WRAM");
	}
	else if (address < 0xFE00)
	{
		// reserved echo RAM
		throw std::exception("Not implemented 'WriteToBus' echo RAM");
	}
	else if (address < 0xFEA0)
	{
		// OAM
		throw std::exception("Not implemented 'WriteToBus' OAM");
	}
	else if (address < 0xFF00)
	{
		// Reserved unused
		throw std::exception("Not implemented 'WriteToBus' Reserved unused");
	}
	else if (address < 0xFF80)
	{
		// I/O registers
		throw std::exception("Not implemented 'WriteToBus' I/O registers");
	}
	else if (address < 0xFF80)
	{
		// HRAM (high RAM)
		throw std::exception("Not implemented 'WriteToBus' HRAM");
	}
	else if (address == 0xFFFF)
	{
		// Interrupt Enable register (IE)
		throw std::exception("Not implemented 'WriteToBus' Interrupt Enable register (IE)");
	}

	throw std::exception(std::format("Not implemented 'WriteToBus' 0x{:x}", address).c_str());
}