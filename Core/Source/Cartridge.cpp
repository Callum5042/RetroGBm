#include "Pch.h"
#include "Cartridge.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <cstdint>
#include <iostream>
#include <string>
#include <numeric>
#include <vector>
#undef max

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
		{ 0x8F, "I�Max" },
		{ 0x91, "Chunsoft Co." },
		{ 0x92, "Video System" },
		{ 0x93, "Tsubaraya Productions Co." },
		{ 0x95, "Varie Corporation" },
		{ 0x96, "Yonezawa/S�Pal" },
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

	std::map<CartridgeType, std::string> cartridge_codes =
	{
		{ CartridgeType::ROM_ONLY, "ROM ONLY" },
		{ CartridgeType::MBC1, "MBC1" },
		{ CartridgeType::MBC1_RAM, "MBC1 + RAM" },
		{ CartridgeType::MBC1_RAM_BATTERY, "MBC1 + RAM + BATTERY" },
		{ CartridgeType::MBC2, "MBC2" },
		{ CartridgeType::MBC2_BATTERY, "MBC2 + BATTERY" },
		{ CartridgeType::ROM_RAM, "ROM + RAM 1" },
		{ CartridgeType::ROM_RAM_BATTERY, "ROM + RAM + BATTERY 1" },
		{ CartridgeType::MMM01, "MMM01" },
		{ CartridgeType::MMM01_RAM, "MMM01 + RAM" },
		{ CartridgeType::MMM01_RAM_BATTERY, "MMM01 + RAM + BATTERY" },
		{ CartridgeType::MBC3_TIMER_BATTERY, "MBC3 + TIMER + BATTERY" },
		{ CartridgeType::MBC3_TIMER_RAM_BATTERY, "MBC3 + TIMER + RAM + BATTERY 2" },
		{ CartridgeType::MBC3, "MBC3" },
		{ CartridgeType::MBC3_RAM, "MBC3 + RAM 2" },
		{ CartridgeType::MBC3_RAM_BATTERY, "MBC3 + RAM + BATTERY 2" },
		{ CartridgeType::MBC5, "MBC5" },
		{ CartridgeType::MBC5_RAM, "MBC5 + RAM" },
		{ CartridgeType::MBC5_RAM_BATTERY, "MBC5 + RAM + BATTERY" },
		{ CartridgeType::MBC5_RUMBLE, "MBC5 + RUMBLE" },
		{ CartridgeType::MBC5_RUMBLE_RAM, "MBC5 + RUMBLE + RAM" },
		{ CartridgeType::MBC5_RUMBLE_RAM_BATTERY, "MBC5 + RUMBLE + RAM + BATTERY" },
		{ CartridgeType::MBC6, "MBC6" },
		{ CartridgeType::MBC7_SENSOR_RUMBLE_RAM_BATTERY, "MBC7 + SENSOR + RUMBLE + RAM + BATTERY" },
		{ CartridgeType::POCKET_CAMERA, "POCKET CAMERA" },
		{ CartridgeType::BANDAI_TAMA5, "BANDAI TAMA5" },
		{ CartridgeType::HuC3, "HuC3" },
		{ CartridgeType::HuC1_RAM_BATTERY, "HuC1 + RAM + BATTERY" }
	};
}

void Cartridge::SetBatteryPath(const std::string& path)
{
	m_BatteryPath = path;
}

bool Cartridge::Load(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		return false;
	}

	std::ifstream file(filepath, std::ios::binary);

	std::vector<uint8_t> data;
	data.clear();
	data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

	return Load(data);
}

bool Cartridge::Load(const std::vector<uint8_t>& filedata)
{
	m_CartridgeInfo.data.clear();
	m_CartridgeInfo.data = filedata;

	// Nintendo logo
	m_CartridgeInfo.header.nintendo_logo.resize(48);
	std::copy(m_CartridgeInfo.data.data() + 0x0104, m_CartridgeInfo.data.data() + 0x0134, m_CartridgeInfo.header.nintendo_logo.data());

	// Title
	m_CartridgeInfo.title.resize(16);
	std::copy(m_CartridgeInfo.data.data() + 0x0134, m_CartridgeInfo.data.data() + 0x0144, m_CartridgeInfo.title.data());

	// Compute title checksum
	m_TitleChecksum = std::accumulate(m_CartridgeInfo.title.begin(), m_CartridgeInfo.title.end(), 0);

	// Manufacturer code
	m_CartridgeInfo.header.manufacturer_code.resize(4);
	std::copy(m_CartridgeInfo.data.data() + 0x013F, m_CartridgeInfo.data.data() + 0x0143, m_CartridgeInfo.header.manufacturer_code.data());

	// Cartridge type
	m_CartridgeInfo.header.cartridge_type_code = static_cast<CartridgeType>(m_CartridgeInfo.data[0x0147]);
	m_CartridgeInfo.header.cartridge_type = cartridge_codes[m_CartridgeInfo.header.cartridge_type_code];

	// Rom size
	uint8_t rom_size = m_CartridgeInfo.data[0x0148];
	m_CartridgeInfo.header.rom_size = 32768 * (1 << rom_size);
	m_CartridgeInfo.header.rom_banks = static_cast<int>(std::pow(2, rom_size + 1));

	// Ram size
	uint8_t ram_size = m_CartridgeInfo.data[0x0149];
	switch (ram_size)
	{
		case 0x02:
			m_CartridgeInfo.header.ram_size = 1024 * 8;
			break;
		case 0x03:
			m_CartridgeInfo.header.ram_size = 1024 * 32;
			break;
		case 0x04:
			m_CartridgeInfo.header.ram_size = 1024 * 128;
			break;
		case 0x05:
			m_CartridgeInfo.header.ram_size = 1024 * 64;
			break;
	}

	// Initialise possible rom banks (8kb each)
	m_CartridgeInfo.external_ram.resize(m_CartridgeInfo.header.ram_size);
	std::fill(m_CartridgeInfo.external_ram.begin(), m_CartridgeInfo.external_ram.end(), 0x0);

	// Destination code
	// TODO

	// Old licensee code
	uint8_t old_licensee_code = m_CartridgeInfo.data[0x014B];
	m_CartridgeInfo.header.old_licensee_code = old_licensee_code;
	m_CartridgeInfo.header.old_licensee = ::license_codes[old_licensee_code];

	// New licensee code
	uint8_t new_licensee_code = m_CartridgeInfo.data[0x0144];
	m_CartridgeInfo.header.new_licensee_code = new_licensee_code;
	m_CartridgeInfo.header.new_licensee = ::license_codes[new_licensee_code];

	// Version
	m_CartridgeInfo.header.version = m_CartridgeInfo.data[0x014C];

	// Check for gameboy colour support
	int title_trim = 0;
	uint8_t cgb_flag = m_CartridgeInfo.data[0x0143];
	if (cgb_flag == 0x80)
	{
		m_CartridgeInfo.header.colour_mode = ColourMode::CGB_SUPPORT;
		title_trim = 1;
	}
	else if (cgb_flag == 0xC0)
	{
		m_CartridgeInfo.header.colour_mode = ColourMode::CGB;
		title_trim = 1;
	}

	// Trim title according to which cartridge
	m_CartridgeInfo.title.erase(m_CartridgeInfo.title.length() - 1);

	// Trim null-terminated char
	if (m_CartridgeInfo.title.find('\0') != std::string::npos)
	{
		m_CartridgeInfo.title.erase(m_CartridgeInfo.title.find('\0'));
	}

	// Load RAM if battery is support
	if (HasBattery())
	{
		std::string filename = m_BatteryPath + m_CartridgeInfo.title + ".save";

		std::ifstream battery(filename, std::ios::in | std::ios::binary);
		battery.read(reinterpret_cast<char*>(&m_CartridgeInfo.ram_bank_controller), 1);
		battery.read(reinterpret_cast<char*>(m_CartridgeInfo.external_ram.data()), m_CartridgeInfo.external_ram.size());

		battery.close();
	}

	return true;
}

bool Cartridge::Checksum(uint8_t* result)
{
	uint8_t checksum = 0;
	for (uint16_t address = 0x0134; address <= 0x014C; address++)
	{
		checksum = checksum - m_CartridgeInfo.data[address] - 1;
	}

	*result = checksum;
	return (checksum & 0xFF);
}

uint8_t Cartridge::Read(uint16_t address)
{
	// Always read from fixed cartridge data 0x0000 to 0x3FFF
	if (m_CartridgeInfo.header.cartridge_type_code == CartridgeType::ROM_ONLY || address <= 0x3FFF)
	{
		if (m_BankingMode == 1)
		{
			if (IsMBC1())
			{
				uint16_t bank_number = m_CartridgeInfo.ram_bank_controller << 5;
				int offset = (address + (0x4000 * bank_number)) % m_CartridgeInfo.data.size();
				return m_CartridgeInfo.data[offset];
			}
			else
			{
				return m_CartridgeInfo.data[address];
			}
		}
		else
		{
			return m_CartridgeInfo.data[address];
		}
	}

	// Read from ROM bank for addresses in range 0x4000 to 0x7FFF
	if (address >= 0x4000 && address <= 0x7FFF)
	{
		if (IsMBC1())
		{
			uint16_t bank_number = m_CartridgeInfo.rom_bank_controller | (m_CartridgeInfo.ram_bank_controller << 5);
			int offset = ((address - 0x4000) + (0x4000 * bank_number)) % m_CartridgeInfo.data.size();
			return m_CartridgeInfo.data[offset];
		}
		else
		{
			uint16_t bank_number = m_CartridgeInfo.rom_bank_controller;
			int offset = ((address - 0x4000) + (0x4000 * bank_number)) % m_CartridgeInfo.data.size();
			return m_CartridgeInfo.data[offset];
		}
	}

	// Read from RAM
	if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (m_CartridgeInfo.enabled_ram)
		{
			uint16_t bank = m_CartridgeInfo.ram_bank_controller;
			if (m_BankingMode == 0)
			{
				bank = 0;
			}

			int offset = ((bank * 0x2000) + address - 0xA000) % m_CartridgeInfo.external_ram.size();
			return m_CartridgeInfo.external_ram[offset];
		}
		else
		{
			return 0xFF;
		}
	}

	std::cout << "Unsupported CartridgeRead 0x" << std::hex << address << '\n';
	return 0xFF;
}

void Cartridge::Write(uint16_t address, uint8_t value)
{
	// Discard writes if cartridge is ROM only
	if (m_CartridgeInfo.header.cartridge_type_code == CartridgeType::ROM_ONLY)
	{
		return;
	}

	// Set RAM
	if (address >= 0x0000 && address <= 0x1FFF)
	{
		// Only enable ram if the lower 4 bits are 0xA otherwise disable ram
		m_CartridgeInfo.enabled_ram = (value & 0xF) == 0xA;

		if (IsMBC3())
		{
			// TODO: Enable/disable RTC
		}

		// Save to file each time we disable the ram
		if (HasBattery() && !m_CartridgeInfo.enabled_ram)
		{
			std::string filename = m_BatteryPath + m_CartridgeInfo.title + ".save";

			std::ofstream battery(filename, std::ios::out | std::ios::binary);
			battery.write(reinterpret_cast<char*>(&m_CartridgeInfo.ram_bank_controller), 1);
			battery.write(reinterpret_cast<char*>(m_CartridgeInfo.external_ram.data()), m_CartridgeInfo.external_ram.size());

			battery.close();
		}

		return;
	}

	// Set ROM bank controller register
	if (address >= 0x2000 && address <= 0x3FFF)
	{
		if (IsMBC1())
		{
			uint8_t bank_number = value & 0x1F;
			if ((bank_number & ~0b00000) == 0)
			{
				bank_number = 0b00001;
			}

			m_CartridgeInfo.rom_bank_controller = bank_number;
		}
		else if (IsMBC3())
		{
			uint8_t bank_number = value & 0b0111'1111;
			if (bank_number == 0x0)
			{
				bank_number = 1;
			}

			m_CartridgeInfo.rom_bank_controller = bank_number;
		}
		else if (IsMBC5())
		{
			if (address >= 0x2000 && address <= 0x2FFF)
			{
				m_CartridgeInfo.rom_bank_controller &= ~0xFF;
				m_CartridgeInfo.rom_bank_controller |= value;
			}
			else if (address >= 0x3000 && address <= 0x3FFF)
			{
				if (value == 0x1)
				{
					m_CartridgeInfo.rom_bank_controller |= 0x100;
				}
				else
				{
					m_CartridgeInfo.rom_bank_controller &= ~0x100;
				}
			}
		}
		else
		{
			// Only the lower 5 bits are used - discard the rest
			m_CartridgeInfo.rom_bank_controller = value & 0x1F;
		}

		// std::cout << "ROM bank selected: " << static_cast<int>(m_CartridgeInfo.rom_bank_controller) << '\n';
		return;
	}

	// Set RAM bank controller register
	if (address >= 0x4000 && address <= 0x5FFF)
	{
		m_CartridgeInfo.ram_bank_controller = value & 0b11;

		if (IsMBC5())
		{
			m_CartridgeInfo.ram_bank_controller = value & 0x0F;
		}

		return;
	}

	if (address >= 0x6000 && address <= 0x7FFF)
	{
		// Banking mode select
		if (IsMBC1())
		{
			m_BankingMode = value & 0x1;
		}

		// Set clock
		if (IsMBC3())
		{
			if (value >= 0x08 && value <= 0x0C)
			{
			}
		}

		return;
	}

	// Write to RAM
	if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (m_CartridgeInfo.enabled_ram && !m_CartridgeInfo.external_ram.empty())
		{
			uint16_t index = m_CartridgeInfo.ram_bank_controller;
			int offset = ((index * 0x2000) + address - 0xA000) % m_CartridgeInfo.external_ram.size();
			m_CartridgeInfo.external_ram[offset] = value;
		}

		return;
	}

	std::cout << "Unsupported CartridgeWrite 0x" << std::hex << address << '\n';
}

bool Cartridge::IsMBC1()
{
	switch (m_CartridgeInfo.header.cartridge_type_code)
	{
		case CartridgeType::MBC1:
		case CartridgeType::MBC1_RAM:
		case CartridgeType::MBC1_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool Cartridge::IsMBC3()
{
	switch (m_CartridgeInfo.header.cartridge_type_code)
	{
		case CartridgeType::MBC3:
		case CartridgeType::MBC3_RAM:
		case CartridgeType::MBC3_RAM_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool Cartridge::IsMBC5()
{
	switch (m_CartridgeInfo.header.cartridge_type_code)
	{
		case CartridgeType::MBC5:
		case CartridgeType::MBC5_RAM:
		case CartridgeType::MBC5_RAM_BATTERY:
		case CartridgeType::MBC5_RUMBLE:
		case CartridgeType::MBC5_RUMBLE_RAM:
		case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool Cartridge::HasBattery()
{
	switch (m_CartridgeInfo.header.cartridge_type_code)
	{
		case CartridgeType::MBC1_RAM_BATTERY:
		case CartridgeType::MBC3_RAM_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY:
		case CartridgeType::MBC5_RAM_BATTERY:
		case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

void Cartridge::SaveState(std::fstream* file)
{
	file->write(reinterpret_cast<const char*>(&m_CartridgeInfo.enabled_ram), sizeof(bool));
	file->write(reinterpret_cast<const char*>(&m_CartridgeInfo.rom_bank_controller), sizeof(m_CartridgeInfo.rom_bank_controller));
	file->write(reinterpret_cast<const char*>(&m_CartridgeInfo.ram_bank_controller), sizeof(m_CartridgeInfo.ram_bank_controller));

	size_t externalram_size = m_CartridgeInfo.external_ram.size();
	file->write(reinterpret_cast<const char*>(&externalram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_CartridgeInfo.external_ram.data()), externalram_size * sizeof(uint8_t));
}

void Cartridge::LoadState(std::fstream* file)
{
	file->read(reinterpret_cast<char*>(&m_CartridgeInfo.enabled_ram), sizeof(bool));
	file->read(reinterpret_cast<char*>(&m_CartridgeInfo.rom_bank_controller), sizeof(m_CartridgeInfo.rom_bank_controller));
	file->read(reinterpret_cast<char*>(&m_CartridgeInfo.ram_bank_controller), sizeof(m_CartridgeInfo.ram_bank_controller));

	size_t external_ram_size = 0;
	file->read(reinterpret_cast<char*>(&external_ram_size), sizeof(size_t));

	m_CartridgeInfo.external_ram.resize(external_ram_size);
	file->read(reinterpret_cast<char*>(m_CartridgeInfo.external_ram.data()), m_CartridgeInfo.external_ram.size() * sizeof(uint8_t));
}

std::string Cartridge::GetColourMode() const
{
	switch (m_CartridgeInfo.header.colour_mode)
	{
		case ColourMode::DMG:
			return "DMG";
		case ColourMode::CGB:
			return "CGB Only";
		case ColourMode::CGB_SUPPORT:
			return "CGB Support";
		default:
			return "Unknown";
	}
}