#pragma once

#include <cstdint>
#include <memory>
#include <vector>

enum class CartridgeTypeV2 : uint8_t
{
	ROM_ONLY = 0x0,
	MBC1 = 0x1,
	MBC1_RAM = 0x02,
	MBC1_RAM_BATTERY = 0x03,
	MBC2 = 0x05,
	MBC2_BATTERY = 0x06,
	ROM_RAM = 0x08,
	ROM_RAM_BATTERY = 0x09,
	MMM01 = 0x0B,
	MMM01_RAM = 0x0C,
	MMM01_RAM_BATTERY = 0x0D,
	MBC3_TIMER_BATTERY = 0x0F,
	MBC3_TIMER_RAM_BATTERY = 0x10,
	MBC3 = 0x11,
	MBC3_RAM = 0x12,
	MBC3_RAM_BATTERY = 0x13,
	MBC5 = 0x19,
	MBC5_RAM = 0x1A,
	MBC5_RAM_BATTERY = 0x1B,
	MBC5_RUMBLE = 0x1C,
	MBC5_RUMBLE_RAM = 0x1D,
	MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	MBC6 = 0x20,
	MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
	POCKET_CAMERA = 0xFC,
	BANDAI_TAMA5 = 0xFD,
	HuC3 = 0xFE,
	HuC1_RAM_BATTERY = 0xFF,
};

struct CartridgeDataV2
{
	CartridgeTypeV2 cartridge_type;
};

class BaseCartridge
{
public:
	BaseCartridge(const CartridgeDataV2 cartridge_data) noexcept;
	virtual ~BaseCartridge() = default;

	virtual uint8_t Read(uint16_t address) = 0;
	virtual uint8_t Write(uint16_t address, uint8_t value) = 0;

	inline const CartridgeDataV2* GetCartridgeData() const
	{
		return &m_CartridgeData;
	}

protected:
	CartridgeDataV2 m_CartridgeData;
};

const CartridgeDataV2 BuildCartridgeData(const std::vector<uint8_t>& filedata);
std::unique_ptr<BaseCartridge> LoadCartridgeFromMemory(const std::vector<uint8_t>& filedata);