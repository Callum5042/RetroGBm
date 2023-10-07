#include "Bus.h"
#include <exception>

const uint8_t ReadFromBus(const CartridgeInfo* cartridge, const uint16_t address)
{
	// TODO: Refactor this into a bus
	if (address < 0x8000)
	{
		// Read from ROM
		return cartridge->data[address];
	}

	throw std::exception("Not implemented");
}