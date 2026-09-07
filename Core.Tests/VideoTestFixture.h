#pragma once
#include "MockCartridge.h"
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/Cpu.h>
#include <filesystem>
#include <vector>

namespace VideoTests
{
class Cartridge : public MockCartridge
{
public:
    explicit Cartridge(bool colour = true) { m_CartridgeData = {}; m_CartridgeData.colour_mode = colour ? ColourModeV2::CGB_SUPPORT : ColourModeV2::DMG; }
    void SetHeader(const std::string& title, uint8_t license = 1)
    {
        m_CartridgeData.data.assign(0x150, 0);
        std::copy_n(title.begin(), std::min(size_t(16), title.size()), m_CartridgeData.data.begin() + 0x134);
        m_CartridgeData.data[0x14B] = license;
        m_CartridgeData.data[0x144] = '0'; m_CartridgeData.data[0x145] = '1';
    }
};
class Output : public IDisplayOutput
{
public:
    int frames = 0, pitch = 0;
    void* buffer = nullptr;
    void Draw(void* data, int stride) override { ++frames; buffer = data; pitch = stride; }
};
struct Fixture
{
    Cartridge cartridge;
    Output output;
    Cpu cpu;
    Display display;
    Ppu ppu;
    explicit Fixture(bool colour = true) : cartridge(colour), cpu(&cartridge), display(&cartridge, &output), ppu(nullptr, &cpu, &display, &cartridge) { display.SetDmgColourisation(false); }
    void Dots(int count) { while (count--) ppu.Tick(); }
    void Off() { display.Write(0xFF40, 0); }
    void On(uint8_t lcdc = 0x91) { display.Write(0xFF40, lcdc); }
    void Tile(uint8_t id, uint8_t low, uint8_t high, uint8_t bank = 0)
    {
        ppu.SetVideoRamBank(bank);
        for (int row = 0; row < 8; ++row) { ppu.WriteVideoRam(0x8000 + id * 16 + row * 2, low); ppu.WriteVideoRam(0x8001 + id * 16 + row * 2, high); }
        ppu.SetVideoRamBank(0);
    }
    void Palette(bool object, uint8_t palette, uint8_t index, uint16_t rgb)
    {
        uint16_t port = object ? 0xFF6A : 0xFF68;
        display.Write(port, uint8_t(0x80 | (palette * 8 + index * 2)));
        display.Write(port + 1, rgb & 255); display.Write(port + 1, rgb >> 8);
    }
    void Object(int i, uint8_t x, uint8_t y, uint8_t tile, uint8_t flags = 0)
    {
        ppu.WriteOamDma(i * 4, y); ppu.WriteOamDma(i * 4 + 1, x); ppu.WriteOamDma(i * 4 + 2, tile); ppu.WriteOamDma(i * 4 + 3, flags);
    }
    uint32_t Pixel(int x, int y = 0) { return static_cast<uint32_t*>(display.GetVideoBuffer())[y * 160 + x]; }
};
inline std::filesystem::path RomRoot()
{
    auto path = std::filesystem::path(__FILE__).parent_path();
    while (!path.empty())
    {
        auto candidate = path / "RetroGBm" / "Resources" / "testroms";
        if (std::filesystem::is_directory(candidate)) return candidate;
        auto parent = path.parent_path(); if (parent == path) break; path = parent;
    }
    return {};
}
}
