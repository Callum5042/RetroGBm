#include <iostream>
#include "Cartridge.h"

// Useful docs
// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// https://rgbds.gbdev.io/docs/v0.5.1/gbz80.7/#HALT
// https://archive.org/details/GameBoyProgManVer1.1/page/n1/mode/2up
// https://gekkio.fi/files/gb-docs/gbctr.pdf

int main(int argc, char** argv)
{
	std::cout << "RetroGBm\n";

	CartridgeInfo cartridge;
	if (!LoadCartridge("Tetris.gb", &cartridge))
	{
		std::cerr << "Unable to load cartidge\n";
		return -1;
	}

	// Print details
	std::cout << "Cartidge loaded\n";
	std::cout << "> Title: " << cartridge.title << '\n';
	std::cout << "> Cartridge Type: " << static_cast<int>(cartridge.header.cartridge_type) << '\n';
	std::cout << "> ROM size: " << cartridge.header.rom_size << '\n';
	std::cout << "> ROM banks: " << cartridge.header.rom_banks << '\n';
	std::cout << "> RAM size: " << cartridge.header.ram_size << '\n';
	std::cout << "> License: " << cartridge.header.license << '\n';
	std::cout << "> Version: " << cartridge.header.version << '\n';

	std::cout << "Checksum: " << (CartridgeChecksum(cartridge) ? "Passed" : "Failed") << '\n' << '\n';

	// Registers
	int program_counter = 0x100;
	int flag_Z = 0;

	int register_A = 0;

	// Loop
	while (true)
	{
		uint8_t op = cartridge.data[program_counter++];

		// NOP
		if (op == 0x0)
		{
			std::cout << "OpCode: " << std::hex << std::showbase << static_cast<int>(op) << '\n';
		}
		else if (op == 0xC3)
		{
			// JP a16
			uint8_t low = cartridge.data[program_counter];
			uint8_t high = cartridge.data[program_counter + 1];
			uint16_t data = low | (high << 8);

			program_counter = data;

			std::cout << (int)op << ": JP\t" << "(" << (int)op << " " << (int)high << " " << (int)low << ")" << '\n';
		}
		else if (op == 0xAF)
		{
			// XOR A, A
			register_A ^= register_A;

			flag_Z = 1;

			std::cout << (int)op << ": XOR\t" << "(" << (int)op <<  ")" << '\n';
		}
		else
		{
			std::cout << "NOT IMPLEMENTED: " << std::hex << std::showbase << static_cast<int>(op) << '\n';
			return -1;
		}
	}

	return 0;
}