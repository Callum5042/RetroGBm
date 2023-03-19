#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <filesystem>
#include <map>

std::unique_ptr<char[]> LoadROM(std::string_view filename, std::size_t& rom_size)
{
	std::filesystem::path filepath(filename);
	if (!std::filesystem::exists(filepath))
	{
		std::cerr << "File doesn't exist\n";
		return {};
	}

	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Failed to open file\n";
		return {};
	}

	// Get the size of the file
	rom_size = file.tellg();
	file.seekg(0, std::ios::beg);

	// Allocate memory for the ROM data
	std::unique_ptr<char[]> data = std::make_unique<char[]>(rom_size);
	if (!data)
	{
		std::cerr << "Failed to allocate memory\n";
		return {};
	}

	// Read the ROM data into memory
	file.read(data.get(), rom_size);

	return std::move(data);
}

int main(int argc, char** argv)
{
	std::cout << "Hello, RetroGBm\n";

	// Load ROM into memory
	std::size_t rom_size = 0;
	std::unique_ptr<char[]> rom_memory = LoadROM("pokemon-crystal.gbc", rom_size);
	if (!rom_memory)
	{  
		std::cerr << "File doesn't exist\n";
		return -1;
	}

	std::cout << "File loaded\n";

	// Memory
	std::unique_ptr<char[]> memory(new char[32768]());
	std::unique_ptr<char[]> video_memory(new char[16384]());

	// Registers
	std::map<std::string, uint16_t> registers;
	registers["AF"] = 0;
	registers["BC"] = 0;
	registers["DE"] = 0;
	registers["HL"] = 0;

	// 16-bit address bus
	// The first 256 bytes of memory are reserved for the system's interrupt vectors and BIOS
	uint16_t program_count = 0x100;

	uint16_t stack_pointer = 0;

	// Main loop
	while (true)
	{
		// Fetch
		uint8_t opcode = rom_memory[program_count];
		program_count++;

		// Decode and execute
		// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
		switch (opcode)
		{
			case 0x00:
				// NOP instruction
				// Do nothing
				break;
			case 0x01:
				// LD BC,nn instruction
				// Load immediate 16-bit value into BC register pair
				registers["BC"] = (rom_memory[static_cast<size_t>(program_count) + 1] << 8) | rom_memory[program_count];
				program_count += 2;
				break;
			case 0x76:
				// HALT instruction
				break;
			default:	
				// Invalid opcode
				std::cerr << "Invalid opcode: 0x" << std::hex << static_cast<int>(opcode) << std::endl;
				break;
		}
	}

	return 0;
}