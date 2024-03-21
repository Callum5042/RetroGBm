#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"

#include "RetroGBm/Cartridge/CartridgeROM.h"
#include "RetroGBm/Cartridge/CartridgeMBC1.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"
#include "RetroGBm/Cartridge/CartridgeMBC5.h"

#include <memory>
#include <vector>
#include <cstdint>
#include <stdexcept>

BaseCartridge::BaseCartridge(const CartridgeDataV2 cartridge_data) noexcept : m_CartridgeData(cartridge_data)
{
}

const CartridgeDataV2 BuildCartridgeData(const std::vector<uint8_t>& filedata)
{
	CartridgeDataV2 data = {};

	// Get cartridge type
	data.cartridge_type = static_cast<CartridgeTypeV2>(filedata[0x0147]);

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