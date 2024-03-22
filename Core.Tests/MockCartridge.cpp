#include "MockCartridge.h"

uint8_t MockCartridge::Read(uint16_t address)
{
	return 0;
}

void MockCartridge::Write(uint16_t address, uint8_t value)
{
}

void MockCartridge::SaveState(std::fstream* file)
{
}

void MockCartridge::LoadState(std::fstream* file)
{
}
