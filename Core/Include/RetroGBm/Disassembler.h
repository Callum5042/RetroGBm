#pragma once

#include <cstdint>
#include <iosfwd>
#include <span>
#include <string>

struct DisassembledInstruction
{
	uint8_t length = 0;
	std::string text;
};

namespace Disassembler
{
	// Decode without executing or accessing the emulator bus. Empty input returns
	// length zero; invalid or incomplete instructions return a one-byte db directive.
	DisassembledInstruction Decode(std::span<const uint8_t> bytes, uint16_t address);

	// Write a linear listing of every physical ROM bank, independent of the
	// currently selected mapper bank. Addresses use the usual $0000/$4000 windows.
	// The cartridge header is emitted as data; other data is decoded as instructions.
	// Includes fixed RGBDS sections and size assertions. RGBDS may pad the output
	// when the original file ends in a partial bank.
	void Export(std::ostream& output, std::span<const uint8_t> rom);
}
