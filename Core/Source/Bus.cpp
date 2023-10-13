#include "Bus.h"
#include <exception>
#include "Cartridge.h"

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

const uint8_t ReadFromBus(EmulatorContext* context, const uint16_t address)
{
	if (address < 0x8000)
	{
		// Read from ROM
		return context->cartridge->data.at(address);
	}
	else if (address < 0xA000)
	{
		// VRAM (video RAM)
		return context->video_ram.at(address - 0x8000);
	}
	else if (address < 0xC000)
	{
		// Read from RAM
		return context->cartridge->data.at(address);
	}
	else if (address < 0x0E000)
	{
		// WRAM (working RAM)
		return context->work_ram.at(address - 0xC000);
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
		if (address == 0xFF01)
		{
			return context->serial_data[0];
		}
		else if (address == 0xFF02)
		{
			return context->serial_data[1];
		}

		throw std::exception("Not implemented 'ReadFromBus' I/O registers");
	}
	else if (address < 0xFFFF)
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

void WriteToBus(EmulatorContext* context, uint16_t address, uint8_t data)
{
	if (address < 0x8000)
	{
		// Write to ROM
		context->cartridge->data.at(address) = data;
		return;
	}
	else if (address < 0xA000)
	{
		// VRAM (video RAM)
		context->video_ram.at(address - 0x8000) = data;
		return;
	}
	else if (address < 0xC000)
	{
		// Write to RAM
		context->cartridge->data.at(address) = data;
		return;
	}
	else if (address < 0x0E000)
	{
		// WRAM (working RAM)
		context->work_ram.at(address - 0xC000) = data;
		return;
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
		if (address == 0xFF01)
		{
			context->serial_data[0] = data;
			return;
		}
		else if (address == 0xFF02)
		{
			context->serial_data[1] = data;
			return;
		}

		throw std::exception("Not implemented 'ReadFromBus' I/O registers");
	}
	else if (address < 0xFFFF)
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