#include "Bus.h"
#include <exception>
#include "Cartridge.h"

#include <Cpu.h>
#include <iostream>

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
		else if (address >= 0xFF04 && address <= 0xFF07)
		{
			switch (address)
			{
				case 0xFF04:
					return context->timer.div >> 8;
				case 0xFF05:
					return context->timer.tima;
				case 0xFF06:
					return context->timer.tma;
				case 0xFF07:
					return context->timer.tac;
			}
		}
		else if (address == 0xFF0F)
		{
			throw std::exception(std::format("Not implemented 'ReadFromBus' I/O registers: 0x{:x}", address).c_str());
		}
		else if (address >= 0xFF40 && address <= 0xFF4B)
		{
			switch (address)
			{
				case 0xFF40:
					return context->display.lcdc;
				case 0xFF41:
					return context->display.stat;
				case 0xFF42:
					return context->display.scy;
				case 0xFF43:
					return context->display.scx;
				case 0xFF44:
					return context->display.ly;
				case 0xFF45:
					return context->display.lyc;
				case 0xFF46:
					return context->display.dma;
				case 0xFF47:
					return context->display.bgp;
				case 0xFF48:
					return context->display.obp0;
				case 0xFF49:
					return context->display.obp1;
				case 0xFF4A:
					return context->display.wy;
				case 0xFF4B:
					return context->display.wx;
			}
		}

		std::cerr << "Unsupported 'ReadFromBus' I/O registers\n";
		return 0;
		// throw std::exception("Not implemented 'ReadFromBus' I/O registers");
	}
	else if (address < 0xFFFF)
	{
		// HRAM (high RAM)
		return context->high_ram[address - 0xFF80];
	}
	else if (address == 0xFFFF)
	{
		// Interrupt Enable register (IE)
		return context->cpu->GetInterruptEnable();
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
		else if (address >= 0xFF04 && address <= 0xFF07)
		{
			switch (address)
			{
				case 0xFF04:
					context->timer.div = 0;
					break;
				case 0xFF05:
					context->timer.tima = data;
					break;
				case 0xFF06:
					context->timer.tma = data;
					break;
				case 0xFF07:
					context->timer.tac = data;
					break;
			}

			return;
		}
		else if (address == 0xFF0F)
		{
			context->cpu->SetInterrupt(data);
			return;
		}
		else if (address >= 0xFF40 && address <= 0xFF4B)
		{
			switch (address)
			{
				case 0xFF40:
					context->display.lcdc = data;
					break;
				case 0xFF41:
					context->display.stat = data;
					break;
				case 0xFF42:
					context->display.scy = data;
					break;
				case 0xFF43:
					context->display.scx = data;
					break;
				case 0xFF44:
					context->display.ly = data;
					break;
				case 0xFF45:
					context->display.lyc = data;
					break;
				case 0xFF46:
					context->display.dma = data;
					break;
				case 0xFF47:
					context->display.bgp = data;
					break;
				case 0xFF48:
					context->display.obp0 = data;
					break;
				case 0xFF49:
					context->display.obp1 = data;
					break;
				case 0xFF4A:
					context->display.wy = data;
					break;
				case 0xFF4B:
					context->display.wx = data;
					break;
			}

			return;
		}

		std::cerr << "Unsupported 'WriteToBus' I/O registers\n";
		// throw std::exception("Not implemented 'WriteToBus' I/O registers");
	}
	else if (address < 0xFFFF)
	{
		// HRAM (high RAM)
		context->high_ram[address - 0xFF80] = data;
		return;
	}
	else if (address == 0xFFFF)
	{
		// Interrupt Enable register (IE)
		context->cpu->InterruptEnable(data);
		return;
	}

	throw std::exception(std::format("Not implemented 'WriteToBus' 0x{:x}", address).c_str());
}