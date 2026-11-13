#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeHeader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <map>

namespace
{
	// The logo bitmap the boot ROM compares against $0104-$0133
	constexpr std::array<uint8_t, 48> HeaderLogo =
	{
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	};

	std::string Unknown(uint8_t code)
	{
		return std::format("Unknown (${:02X})", code);
	}

	// Header text is ASCII; anything else is shown as a space so it can't break the UI
	std::string ReadText(const std::vector<uint8_t>& filedata, size_t start, size_t length)
	{
		std::string text;
		for (size_t i = start; i < start + length; ++i)
		{
			uint8_t c = filedata[i];
			if (c == 0)
			{
				break;
			}

			text.push_back((c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : ' ');
		}

		// Trim trailing spaces
		while (!text.empty() && text.back() == ' ')
		{
			text.pop_back();
		}

		return text;
	}

	bool IsManufacturerCode(const std::string& code)
	{
		if (code.size() != 4)
		{
			return false;
		}

		for (char c : code)
		{
			if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
			{
				return false;
			}
		}

		return true;
	}

	std::string SizeName(int kib)
	{
		if (kib >= 1024 && kib % 1024 == 0)
		{
			return std::format("{} MiB", kib / 1024);
		}
		else if (kib >= 1024)
		{
			return std::format("{:.1f} MiB", kib / 1024.0);
		}

		return std::format("{} KiB", kib);
	}

	std::string BankName(int banks)
	{
		return std::format("{} {}", banks, banks == 1 ? "bank" : "banks");
	}
}

std::optional<CartridgeHeader> ReadCartridgeHeader(const std::vector<uint8_t>& filedata)
{
	if (filedata.size() < CartridgeHeaderEnd)
	{
		return std::nullopt;
	}

	CartridgeHeader header;
	header.cgb_flag = filedata[0x0143];

	// The title was 16 bytes on the first carts. Once the CGB flag took $0143 it was 15,
	// and later carts also gave the last 4 bytes to a manufacturer code.
	const bool has_cgb_flag = (header.cgb_flag & 0x80) != 0;
	header.title = ReadText(filedata, 0x0134, has_cgb_flag ? 15 : 16);

	std::string manufacturer_code = ReadText(filedata, 0x013F, 4);
	if (has_cgb_flag && IsManufacturerCode(manufacturer_code))
	{
		// Those 4 bytes aren't part of the title, which leaves 11
		header.manufacturer_code = manufacturer_code;
		header.title = ReadText(filedata, 0x0134, 11);
	}

	header.new_licensee_code = ReadText(filedata, 0x0144, 2);
	header.sgb_flag = filedata[0x0146];
	header.cartridge_type = filedata[0x0147];
	header.rom_size_code = filedata[0x0148];
	header.ram_size_code = filedata[0x0149];
	header.destination_code = filedata[0x014A];
	header.old_licensee_code = filedata[0x014B];
	header.version = filedata[0x014C];
	header.header_checksum = filedata[0x014D];
	header.global_checksum = static_cast<uint16_t>((filedata[0x014E] << 8) | filedata[0x014F]);

	// Header checksum covers $0134-$014C
	uint8_t checksum = 0;
	for (size_t i = 0x0134; i <= 0x014C; ++i)
	{
		checksum = static_cast<uint8_t>(checksum - filedata[i] - 1);
	}
	header.computed_header_checksum = checksum;

	// Global checksum is every byte in the ROM except the checksum itself
	uint16_t global = 0;
	for (size_t i = 0; i < filedata.size(); ++i)
	{
		if (i != 0x014E && i != 0x014F)
		{
			global = static_cast<uint16_t>(global + filedata[i]);
		}
	}
	header.computed_global_checksum = global;

	header.logo_matches = std::equal(HeaderLogo.begin(), HeaderLogo.end(), filedata.begin() + 0x0104);
	return header;
}

std::string CartridgeHardwareName(const CartridgeHeader& header)
{
	std::string name;
	switch (header.cgb_flag)
	{
		case 0x80:
			name = "Game Boy and Game Boy Color";
			break;
		case 0xC0:
			name = "Game Boy Color only (CGB)";
			break;
		default:
			name = "Game Boy (DMG)";
			break;
	}

	// The SGB only enables its features when the old licensee code is $33
	if (header.sgb_flag == 0x03 && header.UsesNewLicensee())
	{
		name += ", Super Game Boy";
	}

	return name;
}

std::string CartridgeCgbFlagName(uint8_t cgb_flag)
{
	switch (cgb_flag)
	{
		case 0x80:
			return "Works on DMG and CGB";
		case 0xC0:
			return "CGB only";
		default:
			return (cgb_flag & 0x80) ? "CGB, unusual value" : "DMG";
	}
}

std::string CartridgeSgbFlagName(uint8_t sgb_flag)
{
	return sgb_flag == 0x03 ? "Super Game Boy functions" : "No Super Game Boy functions";
}

std::string CartridgeTypeName(uint8_t cartridge_type)
{
	static const std::map<uint8_t, std::string> names =
	{
		{ 0x00, "ROM only" },
		{ 0x01, "MBC1" },
		{ 0x02, "MBC1 + RAM" },
		{ 0x03, "MBC1 + RAM + Battery" },
		{ 0x05, "MBC2" },
		{ 0x06, "MBC2 + Battery" },
		{ 0x08, "ROM + RAM" },
		{ 0x09, "ROM + RAM + Battery" },
		{ 0x0B, "MMM01" },
		{ 0x0C, "MMM01 + RAM" },
		{ 0x0D, "MMM01 + RAM + Battery" },
		{ 0x0F, "MBC3 + Timer + Battery" },
		{ 0x10, "MBC3 + Timer + RAM + Battery" },
		{ 0x11, "MBC3" },
		{ 0x12, "MBC3 + RAM" },
		{ 0x13, "MBC3 + RAM + Battery" },
		{ 0x19, "MBC5" },
		{ 0x1A, "MBC5 + RAM" },
		{ 0x1B, "MBC5 + RAM + Battery" },
		{ 0x1C, "MBC5 + Rumble" },
		{ 0x1D, "MBC5 + Rumble + RAM" },
		{ 0x1E, "MBC5 + Rumble + RAM + Battery" },
		{ 0x20, "MBC6" },
		{ 0x22, "MBC7 + Sensor + Rumble + RAM + Battery" },
		{ 0xFC, "Pocket Camera" },
		{ 0xFD, "Bandai TAMA5" },
		{ 0xFE, "HuC3" },
		{ 0xFF, "HuC1 + RAM + Battery" },
	};

	auto it = names.find(cartridge_type);
	return it != names.end() ? it->second : Unknown(cartridge_type);
}

std::string CartridgeRomSizeName(uint8_t rom_size_code)
{
	if (rom_size_code <= 0x08)
	{
		return SizeName(32 << rom_size_code) + " (" + BankName(2 << rom_size_code) + ")";
	}

	// Sizes listed in some sources but not used by any known cartridge
	switch (rom_size_code)
	{
		case 0x52:
			return SizeName(1152) + " (" + BankName(72) + ")";
		case 0x53:
			return SizeName(1280) + " (" + BankName(80) + ")";
		case 0x54:
			return SizeName(1536) + " (" + BankName(96) + ")";
		default:
			return Unknown(rom_size_code);
	}
}

std::string CartridgeRamSizeName(uint8_t ram_size_code)
{
	switch (ram_size_code)
	{
		case 0x00:
			return "None";
		case 0x01:
			return "Unused code";
		case 0x02:
			return SizeName(8) + " (" + BankName(1) + ")";
		case 0x03:
			return SizeName(32) + " (" + BankName(4) + ")";
		case 0x04:
			return SizeName(128) + " (" + BankName(16) + ")";
		case 0x05:
			return SizeName(64) + " (" + BankName(8) + ")";
		default:
			return Unknown(ram_size_code);
	}
}

std::string CartridgeRamName(const CartridgeHeader& header)
{
	// MBC2 has its RAM built into the mapper and the header says none
	if (header.cartridge_type == 0x05 || header.cartridge_type == 0x06)
	{
		return "512 x 4 bits, built into MBC2";
	}

	return CartridgeRamSizeName(header.ram_size_code);
}

std::string CartridgeDestinationName(uint8_t destination_code)
{
	switch (destination_code)
	{
		case 0x00:
			return "Japan";
		case 0x01:
			return "Overseas";
		default:
			return Unknown(destination_code);
	}
}

std::string CartridgeOldLicenseeName(uint8_t old_licensee_code)
{
	static const std::map<uint8_t, std::string> names =
	{
		{ 0x00, "None" },
		{ 0x01, "Nintendo" },
		{ 0x08, "Capcom" },
		{ 0x09, "HOT-B" },
		{ 0x0A, "Jaleco" },
		{ 0x0B, "Coconuts Japan" },
		{ 0x0C, "Elite Systems" },
		{ 0x13, "EA (Electronic Arts)" },
		{ 0x18, "Hudson Soft" },
		{ 0x19, "ITC Entertainment" },
		{ 0x1A, "Yanoman" },
		{ 0x1D, "Japan Clary" },
		{ 0x1F, "Virgin Games" },
		{ 0x24, "PCM Complete" },
		{ 0x25, "San-X" },
		{ 0x28, "Kemco" },
		{ 0x29, "SETA Corporation" },
		{ 0x30, "Infogrames" },
		{ 0x31, "Nintendo" },
		{ 0x32, "Bandai" },
		{ 0x33, "Uses new licensee code" },
		{ 0x34, "Konami" },
		{ 0x35, "HectorSoft" },
		{ 0x38, "Capcom" },
		{ 0x39, "Banpresto" },
		{ 0x3C, "Entertainment Interactive" },
		{ 0x3E, "Gremlin" },
		{ 0x41, "Ubi Soft" },
		{ 0x42, "Atlus" },
		{ 0x44, "Malibu Interactive" },
		{ 0x46, "Angel" },
		{ 0x47, "Spectrum HoloByte" },
		{ 0x49, "Irem" },
		{ 0x4A, "Virgin Games" },
		{ 0x4D, "Malibu Interactive" },
		{ 0x4F, "U.S. Gold" },
		{ 0x50, "Absolute" },
		{ 0x51, "Acclaim Entertainment" },
		{ 0x52, "Activision" },
		{ 0x53, "Sammy USA Corporation" },
		{ 0x54, "GameTek" },
		{ 0x55, "Park Place" },
		{ 0x56, "LJN" },
		{ 0x57, "Matchbox" },
		{ 0x59, "Milton Bradley Company" },
		{ 0x5A, "Mindscape" },
		{ 0x5B, "Romstar" },
		{ 0x5C, "Naxat Soft" },
		{ 0x5D, "Tradewest" },
		{ 0x60, "Titus Interactive" },
		{ 0x61, "Virgin Games" },
		{ 0x67, "Ocean Software" },
		{ 0x69, "EA (Electronic Arts)" },
		{ 0x6E, "Elite Systems" },
		{ 0x6F, "Electro Brain" },
		{ 0x70, "Infogrames" },
		{ 0x71, "Interplay Entertainment" },
		{ 0x72, "Broderbund" },
		{ 0x73, "Sculptured Software" },
		{ 0x75, "The Sales Curve" },
		{ 0x78, "THQ" },
		{ 0x79, "Accolade" },
		{ 0x7A, "Triffix Entertainment" },
		{ 0x7C, "MicroProse" },
		{ 0x7F, "Kemco" },
		{ 0x80, "Misawa Entertainment" },
		{ 0x83, "LOZC G." },
		{ 0x86, "Tokuma Shoten" },
		{ 0x8B, "Bullet-Proof Software" },
		{ 0x8C, "Vic Tokai" },
		{ 0x8E, "Ape" },
		{ 0x8F, "I'Max" },
		{ 0x91, "Chunsoft" },
		{ 0x92, "Video System" },
		{ 0x93, "Tsuburaya Productions" },
		{ 0x95, "Varie" },
		{ 0x96, "Yonezawa/S'Pal" },
		{ 0x97, "Kemco" },
		{ 0x99, "Arc" },
		{ 0x9A, "Nihon Bussan" },
		{ 0x9B, "Tecmo" },
		{ 0x9C, "Imagineer" },
		{ 0x9D, "Banpresto" },
		{ 0x9F, "Nova" },
		{ 0xA1, "Hori Electric" },
		{ 0xA2, "Bandai" },
		{ 0xA4, "Konami" },
		{ 0xA6, "Kawada" },
		{ 0xA7, "Takara" },
		{ 0xA9, "Technos Japan" },
		{ 0xAA, "Broderbund" },
		{ 0xAC, "Toei Animation" },
		{ 0xAD, "Toho" },
		{ 0xAF, "Namco" },
		{ 0xB0, "Acclaim Entertainment" },
		{ 0xB1, "ASCII Corporation or Nexsoft" },
		{ 0xB2, "Bandai" },
		{ 0xB4, "Square Enix" },
		{ 0xB6, "HAL Laboratory" },
		{ 0xB7, "SNK" },
		{ 0xB9, "Pony Canyon" },
		{ 0xBA, "Culture Brain" },
		{ 0xBB, "Sunsoft" },
		{ 0xBD, "Sony Imagesoft" },
		{ 0xBF, "Sammy Corporation" },
		{ 0xC0, "Taito" },
		{ 0xC2, "Kemco" },
		{ 0xC3, "Square" },
		{ 0xC4, "Tokuma Shoten" },
		{ 0xC5, "Data East" },
		{ 0xC6, "Tonkin House" },
		{ 0xC8, "Koei" },
		{ 0xC9, "UFL" },
		{ 0xCA, "Ultra Games" },
		{ 0xCB, "VAP" },
		{ 0xCC, "Use Corporation" },
		{ 0xCD, "Meldac" },
		{ 0xCE, "Pony Canyon" },
		{ 0xCF, "Angel" },
		{ 0xD0, "Taito" },
		{ 0xD1, "SOFEL" },
		{ 0xD2, "Quest" },
		{ 0xD3, "Sigma Enterprises" },
		{ 0xD4, "ASK Kodansha" },
		{ 0xD6, "Naxat Soft" },
		{ 0xD7, "Copya System" },
		{ 0xD9, "Banpresto" },
		{ 0xDA, "Tomy" },
		{ 0xDB, "LJN" },
		{ 0xDD, "Nippon Computer Systems" },
		{ 0xDE, "Human Entertainment" },
		{ 0xDF, "Altron" },
		{ 0xE0, "Jaleco" },
		{ 0xE1, "Towa Chiki" },
		{ 0xE2, "Yutaka" },
		{ 0xE3, "Varie" },
		{ 0xE5, "Epoch" },
		{ 0xE7, "Athena" },
		{ 0xE8, "Asmik Ace Entertainment" },
		{ 0xE9, "Natsume" },
		{ 0xEA, "King Records" },
		{ 0xEB, "Atlus" },
		{ 0xEC, "Epic/Sony Records" },
		{ 0xEE, "IGS" },
		{ 0xF0, "A Wave" },
		{ 0xF3, "Extreme Entertainment" },
		{ 0xFF, "LJN" },
	};

	auto it = names.find(old_licensee_code);
	return it != names.end() ? it->second : Unknown(old_licensee_code);
}

std::string CartridgeNewLicenseeName(const std::string& new_licensee_code)
{
	static const std::map<std::string, std::string> names =
	{
		{ "00", "None" },
		{ "01", "Nintendo" },
		{ "08", "Capcom" },
		{ "13", "EA (Electronic Arts)" },
		{ "18", "Hudson Soft" },
		{ "19", "B-AI" },
		{ "20", "KSS" },
		{ "22", "Planning Office WADA" },
		{ "24", "PCM Complete" },
		{ "25", "San-X" },
		{ "28", "Kemco" },
		{ "29", "SETA Corporation" },
		{ "30", "Viacom" },
		{ "31", "Nintendo" },
		{ "32", "Bandai" },
		{ "33", "Ocean Software/Acclaim Entertainment" },
		{ "34", "Konami" },
		{ "35", "HectorSoft" },
		{ "37", "Taito" },
		{ "38", "Hudson Soft" },
		{ "39", "Banpresto" },
		{ "41", "Ubi Soft" },
		{ "42", "Atlus" },
		{ "44", "Malibu Interactive" },
		{ "46", "Angel" },
		{ "47", "Bullet-Proof Software" },
		{ "49", "Irem" },
		{ "50", "Absolute" },
		{ "51", "Acclaim Entertainment" },
		{ "52", "Activision" },
		{ "53", "Sammy USA Corporation" },
		{ "54", "Konami" },
		{ "55", "Hi Tech Expressions" },
		{ "56", "LJN" },
		{ "57", "Matchbox" },
		{ "58", "Mattel" },
		{ "59", "Milton Bradley Company" },
		{ "60", "Titus Interactive" },
		{ "61", "Virgin Games" },
		{ "64", "Lucasfilm Games" },
		{ "67", "Ocean Software" },
		{ "69", "EA (Electronic Arts)" },
		{ "70", "Infogrames" },
		{ "71", "Interplay Entertainment" },
		{ "72", "Broderbund" },
		{ "73", "Sculptured Software" },
		{ "75", "The Sales Curve" },
		{ "78", "THQ" },
		{ "79", "Accolade" },
		{ "80", "Misawa Entertainment" },
		{ "83", "LOZC G." },
		{ "86", "Tokuma Shoten" },
		{ "87", "Tsukuda Original" },
		{ "91", "Chunsoft" },
		{ "92", "Video System" },
		{ "93", "Ocean Software/Acclaim Entertainment" },
		{ "95", "Varie" },
		{ "96", "Yonezawa/S'Pal" },
		{ "97", "Kaneko" },
		{ "99", "Pack-In-Video" },
		{ "9H", "Bottom Up" },
		{ "A4", "Konami" },
		{ "BL", "MTO" },
		{ "DK", "Kodansha" },
	};

	auto it = names.find(new_licensee_code);
	if (it != names.end())
	{
		return it->second;
	}

	// Some publishers (Natsume, for one) put their old hex code here instead
	if (new_licensee_code.size() == 2 && std::isxdigit(static_cast<unsigned char>(new_licensee_code[0])) && std::isxdigit(static_cast<unsigned char>(new_licensee_code[1])))
	{
		const uint8_t old_code = static_cast<uint8_t>(std::stoi(new_licensee_code, nullptr, 16));
		if (old_code != 0x33)
		{
			const std::string name = CartridgeOldLicenseeName(old_code);
			if (name.rfind("Unknown", 0) != 0)
			{
				return name;
			}
		}
	}

	return new_licensee_code.empty() ? "Unknown" : "Unknown (" + new_licensee_code + ")";
}

std::string CartridgePublisherName(const CartridgeHeader& header)
{
	return header.UsesNewLicensee()
		? CartridgeNewLicenseeName(header.new_licensee_code)
		: CartridgeOldLicenseeName(header.old_licensee_code);
}

bool CartridgeTypeHasBattery(uint8_t cartridge_type)
{
	switch (cartridge_type)
	{
		case 0x03: case 0x06: case 0x09: case 0x0D: case 0x0F: case 0x10:
		case 0x13: case 0x1B: case 0x1E: case 0x22: case 0xFC: case 0xFE: case 0xFF:
			return true;
		default:
			return false;
	}
}

bool CartridgeTypeHasTimer(uint8_t cartridge_type)
{
	// HuC3 also has a clock
	return cartridge_type == 0x0F || cartridge_type == 0x10 || cartridge_type == 0xFE;
}

bool IsCartridgeTypeSupported(uint8_t cartridge_type)
{
	switch (cartridge_type)
	{
		case 0x00: case 0x08: case 0x09:
		case 0x01: case 0x02: case 0x03:
		case 0x05: case 0x06:
		case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
			return true;
		default:
			return false;
	}
}
