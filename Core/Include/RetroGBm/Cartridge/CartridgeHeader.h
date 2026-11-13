#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Everything in the cartridge header ($0100-$014F), read straight from the ROM bytes.
// Unlike BuildCartridgeData this works for any cartridge type, including ones the
// emulator can't run yet, so it can describe a ROM without loading it.
struct CartridgeHeader
{
	std::string title;
	std::string manufacturer_code;

	uint8_t cgb_flag = 0;
	std::string new_licensee_code;
	uint8_t sgb_flag = 0;
	uint8_t cartridge_type = 0;
	uint8_t rom_size_code = 0;
	uint8_t ram_size_code = 0;
	uint8_t destination_code = 0;
	uint8_t old_licensee_code = 0;
	uint8_t version = 0;

	uint8_t header_checksum = 0;
	uint8_t computed_header_checksum = 0;
	uint16_t global_checksum = 0;
	uint16_t computed_global_checksum = 0;

	bool logo_matches = false;

	inline bool IsHeaderChecksumValid() const { return header_checksum == computed_header_checksum; }
	inline bool IsGlobalChecksumValid() const { return global_checksum == computed_global_checksum; }
	inline bool UsesNewLicensee() const { return old_licensee_code == 0x33; }
};

// Smallest file that holds a full header
constexpr size_t CartridgeHeaderEnd = 0x0150;

// Returns nothing when the file is too small to hold a header
std::optional<CartridgeHeader> ReadCartridgeHeader(const std::vector<uint8_t>& filedata);

// Display text for the header fields
std::string CartridgeHardwareName(const CartridgeHeader& header);
std::string CartridgeCgbFlagName(uint8_t cgb_flag);
std::string CartridgeSgbFlagName(uint8_t sgb_flag);
std::string CartridgeTypeName(uint8_t cartridge_type);
std::string CartridgeRomSizeName(uint8_t rom_size_code);
std::string CartridgeRamSizeName(uint8_t ram_size_code);
std::string CartridgeRamName(const CartridgeHeader& header);
std::string CartridgeDestinationName(uint8_t destination_code);
std::string CartridgeOldLicenseeName(uint8_t old_licensee_code);
std::string CartridgeNewLicenseeName(const std::string& new_licensee_code);
std::string CartridgePublisherName(const CartridgeHeader& header);

bool CartridgeTypeHasBattery(uint8_t cartridge_type);
bool CartridgeTypeHasTimer(uint8_t cartridge_type);

// True when LoadCartridgeFromMemory can build a cartridge for this type
bool IsCartridgeTypeSupported(uint8_t cartridge_type);
