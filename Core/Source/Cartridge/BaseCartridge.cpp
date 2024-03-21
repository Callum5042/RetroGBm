#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Cartridge/CartridgeROM.h"
#include "RetroGBm/Cartridge/CartridgeMBC1.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"
#include "RetroGBm/Cartridge/CartridgeMBC5.h"

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

BaseCartridge::BaseCartridge(const CartridgeDataV2 cartridge_data) noexcept : m_CartridgeData(cartridge_data)
{
	// Compute title checksum
	m_TitleChecksum = std::accumulate(cartridge_data.title.begin(), cartridge_data.title.end(), 0);
}

void BaseCartridge::SetExternalRam(std::vector<uint8_t>&& ram_data)
{
	m_ExternalRam = ram_data;
}

const std::vector<uint8_t>& BaseCartridge::GetExternalRam()
{
	return m_ExternalRam;
}

bool BaseCartridge::HasRAM() const
{
	switch (m_CartridgeData.cartridge_type)
	{
		case CartridgeTypeV2::ROM_RAM:
		case CartridgeTypeV2::ROM_RAM_BATTERY:
		case CartridgeTypeV2::MBC1_RAM:
		case CartridgeTypeV2::MBC1_RAM_BATTERY:
		case CartridgeTypeV2::MBC3_RAM:
		case CartridgeTypeV2::MBC3_RAM_BATTERY:
		case CartridgeTypeV2::MBC3_TIMER_RAM_BATTERY:
		case CartridgeTypeV2::MBC5_RAM:
		case CartridgeTypeV2::MBC5_RAM_BATTERY:
		case CartridgeTypeV2::MBC5_RUMBLE_RAM:
		case CartridgeTypeV2::MBC5_RUMBLE_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool BaseCartridge::HasBattery() const
{
	switch (m_CartridgeData.cartridge_type)
	{
		case CartridgeTypeV2::ROM_RAM_BATTERY:
		case CartridgeTypeV2::MBC1_RAM_BATTERY:
		case CartridgeTypeV2::MBC3_RAM_BATTERY:
		case CartridgeTypeV2::MBC3_TIMER_RAM_BATTERY:
		case CartridgeTypeV2::MBC5_RAM_BATTERY:
		case CartridgeTypeV2::MBC5_RUMBLE_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

const CartridgeDataV2 BuildCartridgeData(const std::vector<uint8_t>& filedata)
{
	CartridgeDataV2 data = { .data = filedata };

	// Nintendo logo
	data.nintendo_logo.resize(48);
	std::copy(filedata.data() + 0x0104, filedata.data() + 0x0134, data.nintendo_logo.data());

	// Title
	data.title.resize(16);
	std::copy(filedata.data() + 0x0134, filedata.data() + 0x0144, data.title.data());

	// Manufacturer code
	data.manufacturer_code.resize(4);
	std::copy(filedata.data() + 0x013F, filedata.data() + 0x0143, data.manufacturer_code.data());

	// Cartridge type
	data.cartridge_type = static_cast<CartridgeTypeV2>(filedata[0x0147]);

	// Rom size
	uint8_t rom_size = filedata[0x0148];
	data.rom_size = 32768 * (1 << rom_size);
	data.rom_banks = static_cast<int>(std::pow(2, rom_size + 1));

	// Ram size
	uint8_t ram_size = filedata[0x0149];
	switch (ram_size)
	{
		case 0x02:
			data.ram_size = 1024 * 8;
			break;
		case 0x03:
			data.ram_size = 1024 * 32;
			break;
		case 0x04:
			data.ram_size = 1024 * 128;
			break;
		case 0x05:
			data.ram_size = 1024 * 64;
			break;
	}

	// Initialise possible rom banks (8kb each)
	/*filedata.external_ram.resize(m_CartridgeInfo.header.ram_size);
	std::fill(m_CartridgeInfo.external_ram.begin(), m_CartridgeInfo.external_ram.end(), 0x0);*/

	// Destination code
	// TODO

	// Old licensee code
	data.old_licensee_code = filedata[0x014B];
	// m_CartridgeInfo.header.old_licensee = ::license_codes[old_licensee_code];

	// New licensee code
	data.new_licensee_code = filedata[0x0144];
	// m_CartridgeInfo.header.new_licensee = ::license_codes[new_licensee_code];

	// Version
	data.version = filedata[0x014C];

	// Check for gameboy colour support
	int title_trim = 0;
	uint8_t cgb_flag = filedata[0x0143];
	if (cgb_flag == 0x80)
	{
		data.colour_mode = ColourModeV2::CGB_SUPPORT;
		title_trim = 1;
	}
	else if (cgb_flag == 0xC0)
	{
		data.colour_mode = ColourModeV2::CGB;
		title_trim = 1;
	}

	// Trim title according to which cartridge
	data.title.erase(data.title.length() - 1);

	// Trim null-terminated char
	if (data.title.find('\0') != std::string::npos)
	{
		data.title.erase(data.title.find('\0'));
	}

	return data;
}

std::unique_ptr<BaseCartridge> LoadCartridgeFromMemory(const std::vector<uint8_t>& filedata)
{
	const CartridgeDataV2 data = BuildCartridgeData(filedata);

	// Build cartridge based on type
	switch (data.cartridge_type)
	{
		case CartridgeTypeV2::ROM_ONLY:
		case CartridgeTypeV2::ROM_RAM:
		case CartridgeTypeV2::ROM_RAM_BATTERY:
			return std::make_unique<CartridgeROM>(data);
		case CartridgeTypeV2::MBC1:
		case CartridgeTypeV2::MBC1_RAM:
		case CartridgeTypeV2::MBC1_RAM_BATTERY:
			return std::make_unique<CartridgeMBC1>(data);
		case CartridgeTypeV2::MBC3:
		case CartridgeTypeV2::MBC3_RAM:
		case CartridgeTypeV2::MBC3_RAM_BATTERY:
			return std::make_unique<CartridgeMBC3>(data);
		case CartridgeTypeV2::MBC5:
		case CartridgeTypeV2::MBC5_RAM:
		case CartridgeTypeV2::MBC5_RAM_BATTERY:
			return std::make_unique<CartridgeMBC5>(data);
		default:
			throw std::runtime_error("Could not determine cartridge from file");
	}
}

const std::string& CartridgeTypeToString(CartridgeTypeV2 cartridge_type)
{
	static const std::string not_found = "N/A";
	static const std::map<CartridgeTypeV2, std::string> cartridge_codes =
	{
		{ CartridgeTypeV2::ROM_ONLY, "ROM ONLY" },
		{ CartridgeTypeV2::MBC1, "MBC1" },
		{ CartridgeTypeV2::MBC1_RAM, "MBC1 + RAM" },
		{ CartridgeTypeV2::MBC1_RAM_BATTERY, "MBC1 + RAM + BATTERY" },
		{ CartridgeTypeV2::MBC2, "MBC2" },
		{ CartridgeTypeV2::MBC2_BATTERY, "MBC2 + BATTERY" },
		{ CartridgeTypeV2::ROM_RAM, "ROM + RAM 1" },
		{ CartridgeTypeV2::ROM_RAM_BATTERY, "ROM + RAM + BATTERY 1" },
		{ CartridgeTypeV2::MMM01, "MMM01" },
		{ CartridgeTypeV2::MMM01_RAM, "MMM01 + RAM" },
		{ CartridgeTypeV2::MMM01_RAM_BATTERY, "MMM01 + RAM + BATTERY" },
		{ CartridgeTypeV2::MBC3_TIMER_BATTERY, "MBC3 + TIMER + BATTERY" },
		{ CartridgeTypeV2::MBC3_TIMER_RAM_BATTERY, "MBC3 + TIMER + RAM + BATTERY 2" },
		{ CartridgeTypeV2::MBC3, "MBC3" },
		{ CartridgeTypeV2::MBC3_RAM, "MBC3 + RAM 2" },
		{ CartridgeTypeV2::MBC3_RAM_BATTERY, "MBC3 + RAM + BATTERY 2" },
		{ CartridgeTypeV2::MBC5, "MBC5" },
		{ CartridgeTypeV2::MBC5_RAM, "MBC5 + RAM" },
		{ CartridgeTypeV2::MBC5_RAM_BATTERY, "MBC5 + RAM + BATTERY" },
		{ CartridgeTypeV2::MBC5_RUMBLE, "MBC5 + RUMBLE" },
		{ CartridgeTypeV2::MBC5_RUMBLE_RAM, "MBC5 + RUMBLE + RAM" },
		{ CartridgeTypeV2::MBC5_RUMBLE_RAM_BATTERY, "MBC5 + RUMBLE + RAM + BATTERY" },
		{ CartridgeTypeV2::MBC6, "MBC6" },
		{ CartridgeTypeV2::MBC7_SENSOR_RUMBLE_RAM_BATTERY, "MBC7 + SENSOR + RUMBLE + RAM + BATTERY" },
		{ CartridgeTypeV2::POCKET_CAMERA, "POCKET CAMERA" },
		{ CartridgeTypeV2::BANDAI_TAMA5, "BANDAI TAMA5" },
		{ CartridgeTypeV2::HuC3, "HuC3" },
		{ CartridgeTypeV2::HuC1_RAM_BATTERY, "HuC1 + RAM + BATTERY" }
	};

	// Look cartridge name up
	auto it = cartridge_codes.find(cartridge_type);
	if (it == cartridge_codes.end())
	{
		return not_found;
	}

	return it->second;
}

std::string ColourModeToString(ColourModeV2 colour_mode)
{
	switch (colour_mode)
	{
		case ColourModeV2::DMG:
			return "DMG";
		case ColourModeV2::CGB:
			return "CGB Only";
		case ColourModeV2::CGB_SUPPORT:
			return "CGB Support";
		default:
			return "Unknown";
	}
}