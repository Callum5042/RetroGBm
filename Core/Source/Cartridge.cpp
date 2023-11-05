#include "Pch.h"
#include "Cartridge.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <cstdint>
#include <iostream>
#undef max

namespace
{
    static std::map<uint8_t, std::string> license_codes = 
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

    static std::map<CartridgeType, std::string> cartridge_codes =
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

bool Cartridge::Load(const std::string& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        return false;
    }

    std::ifstream file(filepath, std::ios::binary);

    m_CartridgeInfo.data.clear();
    m_CartridgeInfo.data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

    // Nintendo logo
    m_CartridgeInfo.header.nintendo_logo.resize(48);
    std::copy(m_CartridgeInfo.data.data() + 0x0104, m_CartridgeInfo.data.data() + 0x0134, m_CartridgeInfo.header.nintendo_logo.data());

    // Title
    m_CartridgeInfo.title.resize(16);
    std::copy(m_CartridgeInfo.data.data() + 0x0134, m_CartridgeInfo.data.data() + 0x0144, m_CartridgeInfo.title.data());

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

    // Destination code
    // TODO

    // Old licensee code
    uint8_t license_code = m_CartridgeInfo.data[0x014B];
    m_CartridgeInfo.header.license_code = license_code;
    m_CartridgeInfo.header.license = ::license_codes[license_code];

    // Version
    m_CartridgeInfo.header.version = m_CartridgeInfo.data[0x014C];

    // Print some info
    std::cout << "Cartridge Type: " << m_CartridgeInfo.header.cartridge_type << '\n';

    // Initialise 125 possible rom banks
    //for (int i = 0; i < 125; ++i)
    //{
    //    // 16kb each
    //    m_CartridgeInfo.rom_banks[i].resize(0x4000);
    //    std::fill(m_CartridgeInfo.rom_banks[i].begin(), m_CartridgeInfo.rom_banks[i].end(), 0x0);
    //}

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
    // Always read from fixed cartridge data 0x0000 to 0x7FFF
    if (m_CartridgeInfo.header.cartridge_type_code == CartridgeType::ROM_ONLY || address < 0x4000)
    {
        return m_CartridgeInfo.data[address];
    }

    // Read from ROM bank for addresses in range 0x4000 to 0x7FFF
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        // std::cout << "Read ROM Bank 0x" << std::hex << address << '\n';
        return m_CartridgeInfo.data[(address - 0x4000) + (0x4000 * std::max<uint8_t>(m_CartridgeInfo.rom_bank_controller, 1))];
    }

    if (address >= 0xA000 && address <= 0xBFFF)
    {
        std::cout << "(RAM) Unsupported CartridgeRead 0x" << std::hex << address << '\n';
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
    if (address >= 0x000 && address <= 0x1FFF)
    {
        // Only enable ram if the lower 4 bits are 0xA otherwise disable ram
        m_CartridgeInfo.enabled_ram = (value & 0xF) == 0xA;
        return;
    }

    // Set ROM bank controller register
    if (address >= 0x2000 && address <= 0x3FFF)
    {
        // Only the lower 5 bits are used - discard the rest
        m_CartridgeInfo.rom_bank_controller = value & 0x1E;
        return;
    }

    std::cout << "Unsupported CartridgeWrite 0x" << std::hex << address << '\n';
    // m_CartridgeInfo.data[address] = value;
}
