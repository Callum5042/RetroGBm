#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <filesystem>

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


	return 0;
}