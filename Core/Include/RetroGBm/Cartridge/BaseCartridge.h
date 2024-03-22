#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <functional>

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

enum class ColourModeV2 : uint8_t
{
	DMG,
	CGB,
	CGB_SUPPORT,
};

struct CartridgeDataV2
{
	std::vector<uint8_t> data;

	// Header data
	std::vector<uint8_t> nintendo_logo;
	CartridgeTypeV2 cartridge_type;
	ColourModeV2 colour_mode;

	uint8_t old_licensee_code = 0;
	uint8_t new_licensee_code = 0;

	int rom_size = 0;
	int rom_banks = 0;
	int ram_size = 0;
	int version = 0;

	std::string title;
	std::string manufacturer_code;
};

class BaseCartridge
{
public:
	BaseCartridge() noexcept;
	BaseCartridge(const CartridgeDataV2 cartridge_data) noexcept;
	virtual ~BaseCartridge() = default;

	virtual uint8_t Read(uint16_t address) = 0;
	virtual void Write(uint16_t address, uint8_t value) = 0;

	virtual void SaveState(std::fstream* file) = 0;
	virtual void LoadState(std::fstream* file) = 0;

	virtual void SetExternalRam(std::vector<uint8_t>&& ram_data);
	virtual const std::vector<uint8_t>& GetExternalRam();

	void SetWriteRamCallback(std::function<void()> writeram_callback);

	bool HasRAM() const;
	bool HasBattery() const;

	inline const CartridgeDataV2& GetCartridgeData() const
	{
		return m_CartridgeData;
	}

	inline uint8_t GetTitleChecksum() const 
	{ 
		return m_TitleChecksum; 
	}

	inline bool IsColourModeDMG() const 
	{ 
		return m_CartridgeData.colour_mode == ColourModeV2::DMG; 
	}

protected:
	CartridgeDataV2 m_CartridgeData;

	std::vector<uint8_t> m_ExternalRam;
	std::function<void()> m_WriteRamCallback;

private:
	uint8_t m_TitleChecksum = 0;
};

const CartridgeDataV2 BuildCartridgeData(const std::vector<uint8_t>& filedata);
std::unique_ptr<BaseCartridge> LoadCartridgeFromMemory(const std::vector<uint8_t>& filedata);

const std::string& CartridgeTypeToString(CartridgeTypeV2 cartridge_type);
std::string ColourModeToString(ColourModeV2 colour_mode);