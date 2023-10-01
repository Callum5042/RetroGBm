#include "Cartridge.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <cstdint>

namespace
{
    std::map<uint8_t, std::string> license_codes = 
    {
        { 0x00, "None" },
        { 0x01, "Nintendo" },
        { 0x08, "Capcom" },
        { 0x09, "Hot-B" },
        { 0x0A, "Jaleco" },
        { 0x0B, "Coconuts Japan" },
        { 0x0C, "Elite Systems" },
        { 0x13, "EA (Electronic Arts)" },
        { 0x18, "Hudsonsoft" },
        { 0x19, "ITC Entertainment" },
        { 0x1A, "Yanoman" },
        { 0x1D, "Japan Clary" },
        { 0x1F, "Virgin Interactive" },
        { 0x24, "PCM Complete" },
        { 0x25, "San-X" },
        { 0x28, "Kotobuki Systems" },
        { 0x29, "Seta" },
        { 0x30, "Infogrames" },
        { 0x31, "Nintendo" },
        { 0x32, "Bandai" },
        { 0x33, "Indicates that the New licensee code should be used instead." },
        { 0x34, "Konami" },
        { 0x35, "HectorSoft" },
        { 0x38, "Capcom" },
        { 0x39, "Banpresto" },
        { 0x3C, ".Entertainment i" },
        { 0x3E, "Gremlin" },
        { 0x41, "Ubisoft" },
        { 0x42, "Atlus" },
        { 0x44, "Malibu" },
        { 0x46, "Angel" },
        { 0x47, "Spectrum Holoby" },
        { 0x49, "Irem" },
        { 0x4A, "Virgin Interactive" },
        { 0x4D, "Malibu" },
        { 0x4F, "U.S. Gold" },
        { 0x50, "Absolute" },
        { 0x51, "Acclaim" },
        { 0x52, "Activision" },
        { 0x53, "American Sammy" },
        { 0x54, "GameTek" },
        { 0x55, "Park Place" },
        { 0x56, "LJN" },
        { 0x57, "Matchbox" },
        { 0x59, "Milton Bradley" },
        { 0x5A, "Mindscape" },
        { 0x5B, "Romstar" },
        { 0x5C, "Naxat Soft" },
        { 0x5D, "Tradewest" },
        { 0x60, "Titus" },
        { 0x61, "Virgin Interactive" },
        { 0x67, "Ocean Interactive" },
        { 0x69, "EA (Electronic Arts)" },
        { 0x6E, "Elite Systems" },
        { 0x6F, "Electro Brain" },
        { 0x70, "Infogrames" },
        { 0x71, "Interplay" },
        { 0x72, "Broderbund" },
        { 0x73, "Sculptered Soft" },
        { 0x75, "The Sales Curve" },
        { 0x78, "t.hq" },
        { 0x79, "Accolade" },
        { 0x7A, "Triffix Entertainment" },
        { 0x7C, "Microprose" },
        { 0x7F, "Kemco" },
        { 0x80, "Misawa Entertainment" },
        { 0x83, "Lozc" },
        { 0x86, "Tokuma Shoten Intermedia" },
        { 0x8B, "Bullet-Proof Software" },
        { 0x8C, "Vic Tokai" },
        { 0x8E, "Ape" },
        { 0x8F, "I’Max" },
        { 0x91, "Chunsoft Co." },
        { 0x92, "Video System" },
        { 0x93, "Tsubaraya Productions Co." },
        { 0x95, "Varie Corporation" },
        { 0x96, "Yonezawa/S’Pal" },
        { 0x97, "Kaneko" },
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
        { 0xB0, "acclaim" },
        { 0xB1, "ASCII or Nexsoft" },
        { 0xB2, "Bandai" },
        { 0xB4, "Square Enix" },
        { 0xB6, "HAL Laboratory" },
        { 0xB7, "SNK" },
        { 0xB9, "Pony Canyon" },
        { 0xBA, "Culture Brain" },
        { 0xBB, "Sunsoft" },
        { 0xBD, "Sony Imagesoft" },
        { 0xBF, "Sammy" },
        { 0xC0, "Taito" },
        { 0xC2, "Kemco" },
        { 0xC3, "Squaresoft" },
        { 0xC4, "Tokuma Shoten Intermedia" },
        { 0xC5, "Data East" },
        { 0xC6, "Tonkinhouse" },
        { 0xC8, "Koei" },
        { 0xC9, "UFL" },
        { 0xCA, "Ultra" },
        { 0xCB, "Vap" },
        { 0xCC, "Use Corporation" },
        { 0xCD, "Meldac" },
        { 0xCE, ".Pony Canyon or" },
        { 0xCF, "Angel" },
        { 0xD0, "Taito" },
        { 0xD1, "Sofel" },
        { 0xD2, "Quest" },
        { 0xD3, "Sigma Enterprises" },
        { 0xD4, "ASK Kodansha Co." },
        { 0xD6, "Naxat Soft" },
        { 0xD7, "Copya System" },
        { 0xD9, "Banpresto" },
        { 0xDA, "Tomy" },
        { 0xDB, "LJN" },
        { 0xDD, "NCS" },
        { 0xDE, "Human" },
        { 0xDF, "Altron" },
        { 0xE0, "Jaleco" },
        { 0xE1, "Towa Chiki" },
        { 0xE2, "Yutaka" },
        { 0xE3, "Varie" },
        { 0xE5, "Epcoh" },
        { 0xE7, "Athena" },
        { 0xE8, "Asmik ACE Entertainment" },
        { 0xE9, "Natsume" },
        { 0xEA, "King Records" },
        { 0xEB, "Atlus" },
        { 0xEC, "Epic/Sony Records" },
        { 0xEE, "IGS" },
        { 0xF0, "A Wave" },
        { 0xF3, "Extreme Entertainment" },
        { 0xFF, "LJN" }
    };
}

bool LoadCartridge(const std::filesystem::path& path, CartridgeInfo* cartridge_info)
{
	if (!std::filesystem::exists(path))
	{
		return false;
	}

	std::ifstream file(path, std::ios::binary);

	cartridge_info->data.clear();
	cartridge_info->data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

	// Title
	cartridge_info->title.resize(16);
	std::copy(cartridge_info->data.data() + 0x0134, cartridge_info->data.data() + 0x0143, cartridge_info->title.data());

	// Manufacturer code
	cartridge_info->header.manufacturer_code.resize(4);
	std::copy(cartridge_info->data.data() + 0x013F, cartridge_info->data.data() + 0x0142, cartridge_info->header.manufacturer_code.data());

	// Cartridge type
	cartridge_info->header.cartridge_type = cartridge_info->data[0x0147];

	// Rom size
	uint8_t rom_size = cartridge_info->data[0x0148];
	cartridge_info->header.rom_size = 32768 * (1 << rom_size);
	cartridge_info->header.rom_banks = static_cast<int>(std::pow(2, rom_size + 1));

	// Ram size
	uint8_t ram_size = cartridge_info->data[0x0149];
	switch (ram_size)
	{
		case 0x02:
			cartridge_info->header.ram_size = 1024 * 8;
			break;
		case 0x03:
			cartridge_info->header.ram_size = 1024 * 32;
			break;
		case 0x04:
			cartridge_info->header.ram_size = 1024 * 128;
			break;
		case 0x05:
			cartridge_info->header.ram_size = 1024 * 64;
			break;
	}

	// Destination code
	// TODO
	
	// Old licensee code
	uint8_t license_code = cartridge_info->data[0x014B];
	cartridge_info->header.license_code = license_code;
    cartridge_info->header.license = ::license_codes[license_code];

	// Version
	cartridge_info->header.version = cartridge_info->data[0x014C];


	return true;
}