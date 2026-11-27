#pragma once
#include <array>
#include <deque>
#include <chrono>
#include <cstdint>
#include <fstream>
#include "RetroGBm/Display.h"

class IBus;
class Cpu;
class BaseCartridge;

struct VideoPixel
{
    uint8_t colour = 0;
    uint8_t palette = 0;
    uint8_t priority = 0;
    uint8_t order = 255; 
    int16_t objectX = 256;
};

struct LineObject 
{ 
    uint8_t y = 0;
    uint8_t x = 0;
    uint8_t tile = 0;
    uint8_t flags = 0;
    uint8_t order = 0;
};

class Ppu
{
public:
    Ppu() = default;
    Ppu(IBus* bus, Cpu* cpu, Display* display, BaseCartridge* cartridge);
    void Init();

    void Tick();

    void LcdEnableChanged();

    void LcdcWritten(uint8_t previous, uint8_t value);

    void UpdateStat();

    uint8_t ReadVideoRam(uint16_t address) const;
    uint8_t ReadVideoRam(uint16_t address, uint8_t bank) const;

    void WriteVideoRam(uint16_t address, uint8_t value);
    void WriteVideoRamDma(uint16_t address, uint8_t value);

    uint8_t ReadOam(uint16_t address) const;
    void WriteOam(uint16_t address, uint8_t value);
    void WriteOamDma(uint16_t address, uint8_t value);

    // All of OAM for debug views, readable whatever mode the PPU is in
    const std::array<uint8_t, 160>& GetOam() const { return m_Oam; }

    // Both VRAM banks, 8 KiB each, for debug views
    const std::array<uint8_t, 0x4000>& GetVideoRam() const { return m_Vram; }

    // Debug views: set a byte of VRAM (an offset into both banks) or OAM, whatever mode the
    // PPU is in or OAM DMA is doing. Offsets past the end are ignored.
    void PokeVideoRam(size_t offset, uint8_t value) { if (offset < m_Vram.size()) m_Vram[offset] = value; }
    void PokeOam(size_t offset, uint8_t value) { if (offset < m_Oam.size()) m_Oam[offset] = value; }

    uint8_t GetVideoRamBank() const;
    void SetVideoRamBank(uint8_t value);

    bool ConsumeHBlank() { bool value = m_HBlank; m_HBlank = false; return value; }
    bool ConsumeFrame() { bool value = m_Frame; m_Frame = false; return value; }

    void SetSpeedMultipler(float value);
    void PaceFrame();

    int GetFPS() const { return m_Fps; }
    uint16_t GetDot() const { return m_Dot; }

    void SaveState(std::fstream* file);
    void LoadState(std::fstream* file);

private:
    void BeginLine(); void BeginTransfer(); void Transfer(); void Fetch(); void MergeObject();
    void SetMode(LcdMode mode); void ResetClock();
    Display* m_Display = nullptr;
    Cpu* m_Cpu = nullptr;
    std::array<uint8_t, 0x4000> m_Vram{};
    std::array<uint8_t, 160> m_Oam{};
    std::array<LineObject, 10> m_Objects{};
    std::array<uint8_t, 10> m_Fetched{};
    std::deque<VideoPixel> m_BackgroundFifo;
    std::array<VideoPixel, 8> m_ObjectFifo{};
    uint16_t m_Dot = 0, m_TransferDots = 0;
    uint8_t m_Line = 0, m_Bank = 0, m_Count = 0, m_WindowLine = 0;
    uint8_t m_FirstLine = 0;
    uint8_t m_WindowTriggered = 0, m_WindowActive = 0, m_WindowUsed = 0, m_StatLine = 0;
    uint8_t m_FetchPhase = 0, m_FetchX = 0, m_Tile = 0, m_Attribute = 0, m_Low = 0, m_High = 0;
    uint16_t m_TileAddress = 0;
    int16_t m_X = 0;
    int16_t m_LastObjectTile = -1;
    uint8_t m_Discard = 0, m_Startup = 0, m_ObjectStall = 0, m_CurrentObject = 0;
    bool m_HBlank = false, m_Frame = false;
    double m_Speed = 1;
    int m_Fps = 0, m_FrameCount = 0;
    std::chrono::steady_clock::time_point m_Deadline{}, m_FpsStart{};
};
