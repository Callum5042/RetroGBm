#pragma once

#include <cstdint>
#include <string>

struct GamesharkToken
{
	uint8_t bank = 0;
	uint8_t value = 0;
	uint16_t address = 0;
};

GamesharkToken ParseGamesharkCode(const std::string& code);

