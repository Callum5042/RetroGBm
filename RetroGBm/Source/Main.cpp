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

	std::cout << "Checksum: " << (CartridgeChecksum(cartridge) ? "Passed" : "Failed") << '\n';

	return 0;
}