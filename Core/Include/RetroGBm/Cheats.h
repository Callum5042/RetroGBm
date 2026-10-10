#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GamesharkToken
{
	uint8_t bank = 0;
	uint8_t value = 0;
	uint16_t address = 0;
};

GamesharkToken ParseGamesharkCode(const std::string& code);

struct GamesharkValidation
{
	// True when there is at least one code and every non-blank line is a valid code
	bool valid = false;

	// Valid codes found, uppercased with whitespace removed
	std::vector<std::string> codes;

	// 1-based line number of the first invalid line, 0 if none
	size_t error_line = 0;

	// The first invalid line, trimmed
	std::string error_text;
};

// Validates GameShark codes entered one per line (LF or CRLF). Blank lines are skipped,
// whitespace is ignored and each remaining line must be exactly 8 hex digits.
GamesharkValidation ValidateGamesharkCodes(const std::string& text);
