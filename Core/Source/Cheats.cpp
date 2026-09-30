#include "RetroGBm/Pch.h"
#include "RetroGBm/Cheats.h"
#include <cstdlib>

GamesharkToken ParseGamesharkCode(const std::string& code)
{
	// Convert string to hex value
	char* endptr;
	uint32_t value = static_cast<uint32_t>(strtoul(code.c_str(), &endptr, 16));

	// Extract the values
	GamesharkToken token;
	token.bank = (value >> 24) & 0xFF;
	token.value = (value >> 16) & 0xFF;
	token.address = ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);

	return token;
}