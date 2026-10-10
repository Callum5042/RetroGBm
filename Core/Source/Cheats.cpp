#include "RetroGBm/Pch.h"
#include "RetroGBm/Cheats.h"
#include <cctype>
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

GamesharkValidation ValidateGamesharkCodes(const std::string& text)
{
	GamesharkValidation result;

	size_t line_number = 0;
	size_t start = 0;
	while (start <= text.size())
	{
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
		{
			end = text.size();
		}

		const std::string line = text.substr(start, end - start);
		start = end + 1;
		++line_number;

		// Remove all whitespace, including the '\r' of CRLF line endings
		std::string code;
		for (char c : line)
		{
			if (!std::isspace(static_cast<unsigned char>(c)))
			{
				code += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			}
		}

		if (code.empty())
		{
			continue;
		}

		bool is_hex = true;
		for (char c : code)
		{
			if (!std::isxdigit(static_cast<unsigned char>(c)))
			{
				is_hex = false;
				break;
			}
		}

		if (code.size() != 8 || !is_hex)
		{
			// Report the line as typed, without surrounding whitespace
			size_t first = line.find_first_not_of(" \t\r");
			size_t last = line.find_last_not_of(" \t\r");

			result.valid = false;
			result.codes.clear();
			result.error_line = line_number;
			result.error_text = line.substr(first, last - first + 1);
			return result;
		}

		result.codes.push_back(code);
	}

	result.valid = !result.codes.empty();
	return result;
}
