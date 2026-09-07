#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include "RetroGBm/IDisplayOutput.h"

class BaseCartridge;
class Ppu;
enum class LcdMode : uint8_t { HBlank, VBlank, OAM, PixelTransfer };

class Display
{
public:
    static constexpr int ScreenResolutionX = 160;
    static constexpr int ScreenResolutionY = 144;
    explicit Display(IDisplayOutput* output);
    Display(BaseCartridge* cartridge, IDisplayOutput* output);
    void AttachPpu(Ppu* ppu) { m_Ppu = ppu; }
    void Init(bool bootRom = false);
    bool IsColour() const { return m_Colour; }
    void SetDmgColourisation(bool enabled) { m_DmgColourisation = enabled; }
    bool GetDmgColourisation() const { return m_DmgColourisation; }
    uint8_t Read(uint16_t address) const;
    void Write(uint16_t address, uint8_t value);
    bool IsLcdEnabled() const { return (m_Registers[0] & 0x80) != 0; }
    LcdMode GetLcdMode() const { return static_cast<LcdMode>(m_Registers[1] & 3); }
    void SetStatus(LcdMode mode, uint8_t ly, bool compare = true, bool suppressCoincidence = false);
    uint8_t GetObjectPriorityMode() const { return m_Colour ? uint8_t(0xFE | m_Priority) : 0xFF; }
    void SetObjectPriorityMode(uint8_t value) { if (m_Colour) m_Priority = value & 1; }
    bool CoordinatePriority() const { return !m_Colour || m_Priority; }
    uint32_t GetColourFromBackgroundPalette(uint8_t palette, uint8_t index) const;
    uint32_t GetColourFromObjectPalette(uint8_t palette, uint8_t index) const;
    void SetVideoBufferPixel(int x, int y, uint32_t value);
    void UpdateDisplay();
    void Blank();
    void* GetVideoBuffer() { return m_Front.data(); }
    int GetVideoBufferSize() const { return ScreenResolutionX * ScreenResolutionY; }
    void SaveState(std::fstream* file);
    void LoadState(std::fstream* file);
private:
    void InitCompatibilityPalettes();
    BaseCartridge* m_Cartridge = nullptr;
    bool m_DmgColourisation = true;
    uint32_t Colour(const std::array<uint8_t, 64>& ram, uint8_t palette, uint8_t index) const;
    bool m_Colour = false;
    IDisplayOutput* m_Output = nullptr;
    Ppu* m_Ppu = nullptr;
    std::array<uint8_t, 12> m_Registers{};
    std::array<uint8_t, 64> m_Background{}, m_Objects{};
    uint8_t m_BgIndex = 0, m_ObjIndex = 0, m_Priority = 0;
    std::array<uint32_t, ScreenResolutionX * ScreenResolutionY> m_Front{}, m_Back{};
};
