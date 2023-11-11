#pragma once

#include <string>
#include <filesystem>
#include <vector>

enum class CartridgeType : uint8_t
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

struct CartridgeHeader
{
	std::string manufacturer_code;
	CartridgeType cartridge_type_code;
	std::string cartridge_type;
	int rom_size = 0;
	int rom_banks = 0;
	int ram_size = 0;
	int version = 0;
	int license_code = 0;
	std::string license;
	std::vector<uint8_t> nintendo_logo;
};

struct CartridgeInfo
{
	std::string title;
	std::vector<uint8_t> data;
	CartridgeHeader header;

	bool enabled_ram = false;
	uint8_t rom_bank_controller = 1;
	uint8_t* rom_bank_memory = nullptr;

	uint8_t ram_bank_controller = 0;
	std::vector<uint8_t> ram_banks[3];
};

class Cartridge
{
public:
	Cartridge() = default;
	virtual ~Cartridge() = default;

	bool Load(const std::string& filepath);
	bool Checksum(uint8_t* result);

	uint8_t Read(uint16_t address);
	void Write(uint16_t address, uint8_t value);

	inline const CartridgeInfo* GetCartridgeInfo() { return &m_CartridgeInfo; }

private:
	CartridgeInfo m_CartridgeInfo;

	bool IsMBC1();
	bool IsMBC3();

	bool HasBattery();
};