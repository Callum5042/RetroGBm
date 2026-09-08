#include "RetroGBm/Pch.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"
#include "RetroGBm/VideoState.h"
#include "RetroGBm/CompatibilityPalettes.h"

Display::Display(IDisplayOutput* output) : Display(nullptr, output)
{}

Display::Display(BaseCartridge* cartridge, IDisplayOutput* output) : m_Cartridge(cartridge), m_Colour(cartridge && !cartridge->IsColourModeDMG()), m_Output(output)
{
	Init();
}

void Display::Init(bool bootRom)
{
	m_Registers = {};
	m_Registers[0] = bootRom ? 0 : 0x91;
	m_Registers[1] = 0x80;
	m_Registers[7] = 0xFC;
	m_Registers[8] = m_Registers[9] = 0xFF;

	m_Background.fill(0xFF);
	m_Objects.fill(0xFF);

	if (!m_Colour)
	{
		InitCompatibilityPalettes();
	}

	m_BgIndex = 0;
	m_ObjIndex = 0;
	m_Priority = 0;

	m_Front.fill(0xFFFFFFFF);
	m_Back.fill(0xFFFFFFFF);
}

uint8_t Display::Read(uint16_t address) const
{
	if (address >= 0xFF40 && address <= 0xFF4B)
	{
		return m_Registers[address - 0xFF40];
	}

	if (!m_Colour)
	{
		return 0xFF;
	}

	switch (address)
	{
		case 0xFF68:
			return m_BgIndex | 0x40;
		case 0xFF6A:
			return m_ObjIndex | 0x40;
		case 0xFF69:
			return GetLcdMode() == LcdMode::PixelTransfer && IsLcdEnabled() ? 0xFF : m_Background[m_BgIndex & 63];
		case 0xFF6B:
			return GetLcdMode() == LcdMode::PixelTransfer && IsLcdEnabled() ? 0xFF : m_Objects[m_ObjIndex & 63];
		case 0xFF6C:
			return GetObjectPriorityMode();
		default:
			return 0xFF;
	}
}

void Display::Write(uint16_t address, uint8_t value)
{
	if (address >= 0xFF40 && address <= 0xFF4B)
	{
		bool enabled = IsLcdEnabled();
		if (address == 0xFF44)
		{
			return;
		}

		if (address == 0xFF41)
		{
			value = uint8_t(0x80 | (value & 0x78) | (m_Registers[1] & 7));
		}

		m_Registers[address - 0xFF40] = value;

		if (m_Ppu)
		{
			if (address == 0xFF40 && enabled != IsLcdEnabled()) m_Ppu->LcdEnableChanged();
			if (address == 0xFF41 || address == 0xFF45) m_Ppu->UpdateStat();
		}

		return;
	}

	if (!m_Colour)
	{
		return;
	}

	switch (address)
	{
		case 0xFF68:
			m_BgIndex = value & 0xBF;
			break;
		case 0xFF6A:
			m_ObjIndex = value & 0xBF;
			break;
		case 0xFF69:
		case 0xFF6B:
		{
			auto& index = address == 0xFF69 ? m_BgIndex : m_ObjIndex;
			auto& ram = address == 0xFF69 ? m_Background : m_Objects;
			if (!IsLcdEnabled() || GetLcdMode() != LcdMode::PixelTransfer)
			{
				ram[index & 63] = value;
			}

			if (index & 0x80)
			{
				index = uint8_t(0x80 | ((index + 1) & 63));
			}

			break;
		}
		case 0xFF6C:
			SetObjectPriorityMode(value);
			break;
		default:
			break;
	}
}

void Display::SetStatus(LcdMode mode, uint8_t ly, bool compare, bool suppressCoincidence)
{
	m_Registers[4] = ly;
	uint8_t coincidence = compare ? (ly == m_Registers[5] ? 4 : 0) : m_Registers[1] & 4;
	if (suppressCoincidence)
	{
		coincidence = 0;
	}

	m_Registers[1] = uint8_t((m_Registers[1] & 0xF8) | static_cast<uint8_t>(mode) | coincidence);
}

uint32_t Display::Colour(const std::array<uint8_t, 64>& ram, uint8_t palette, uint8_t index) const
{
	size_t offset = (palette & 7) * 8 + (index & 3) * 2;
	uint16_t rgb = ram[offset] | (uint16_t(ram[offset + 1]) << 8);
	auto expand = [](uint32_t channel) { return (channel << 3) | (channel >> 2); };
	return 0xFF000000 | expand(rgb & 31) | (expand((rgb >> 5) & 31) << 8) | (expand((rgb >> 10) & 31) << 16);
}

uint32_t Display::GetColourFromBackgroundPalette(uint8_t palette, uint8_t index) const
{
	if (m_Colour)
	{
		return Colour(m_Background, palette, index);
	}

	if (m_DmgColourisation)
	{
		return Colour(m_Background, 0, (m_Registers[7] >> ((index & 3) * 2)) & 3);
	}

	uint32_t shade = 255 - 85 * ((m_Registers[7] >> ((index & 3) * 2)) & 3);
	return 0xFF000000 | shade * 0x010101;
}

uint32_t Display::GetColourFromObjectPalette(uint8_t palette, uint8_t index) const
{
	if (m_Colour)
	{
		return Colour(m_Objects, palette, index);
	}

	if (m_DmgColourisation)
	{
		return Colour(m_Objects, palette & 1, (m_Registers[8 + (palette & 1)] >> ((index & 3) * 2)) & 3);
	}

	uint32_t shade = 255 - 85 * ((m_Registers[8 + (palette & 1)] >> ((index & 3) * 2)) & 3);
	return 0xFF000000 | shade * 0x010101;
}

void Display::SetVideoBufferPixel(int x, int y, uint32_t value)
{
	if (x >= 0 && x < ScreenResolutionX && y >= 0 && y < ScreenResolutionY)
	{
		m_Back[y * ScreenResolutionX + x] = value;
	}
}

void Display::UpdateDisplay()
{
	std::copy(m_Back.begin(), m_Back.end(), m_Front.begin());
	if (m_Output)
	{
		m_Output->Draw(m_Front.data(), ScreenResolutionX * sizeof(uint32_t));
	}
}

void Display::Blank()
{
	m_Back.fill(0xFFFFFFFF);
	UpdateDisplay();
}

void Display::SaveState(std::fstream* file)
{
	VideoState::Write(*file, m_DmgColourisation);
	for (auto value : m_Registers) VideoState::Write(*file, value);
	for (auto value : m_Background) VideoState::Write(*file, value);
	for (auto value : m_Objects) VideoState::Write(*file, value);
	VideoState::Write(*file, m_BgIndex); VideoState::Write(*file, m_ObjIndex); VideoState::Write(*file, m_Priority);
	for (auto value : m_Front) VideoState::Write(*file, value);
	for (auto value : m_Back) VideoState::Write(*file, value);
}

void Display::LoadState(std::fstream* file)
{
	VideoState::Read(*file, m_DmgColourisation);
	for (auto& value : m_Registers) VideoState::Read(*file, value);
	for (auto& value : m_Background) VideoState::Read(*file, value);
	for (auto& value : m_Objects) VideoState::Read(*file, value);
	VideoState::Read(*file, m_BgIndex); VideoState::Read(*file, m_ObjIndex); VideoState::Read(*file, m_Priority);
	for (auto& value : m_Front) VideoState::Read(*file, value);
	for (auto& value : m_Back) VideoState::Read(*file, value);
	if (m_Output) m_Output->Draw(m_Front.data(), ScreenResolutionX * sizeof(uint32_t));
}

void Display::InitCompatibilityPalettes()
{
	using namespace CompatibilityPalettes;
	uint8_t combination = 0;

	if (m_Cartridge)
	{
		const auto& rom = m_Cartridge->GetCartridgeData().data;
		if (rom.size() >= 0x150 && (rom[0x14B] == 1 || (rom[0x14B] == 0x33 && rom[0x144] == '0' && rom[0x145] == '1')))
		{
			uint8_t checksum = 0;
			for (int i = 0x134; i <= 0x143; ++i) 
			{ 
				checksum += rom[i]; 
			}

			for (size_t i = 0; i < sizeof(Checksums); ++i)
			{
				if (Checksums[i] == checksum && (i < 65 || FourthLetters[i - 65] == rom[0x137]))
				{
					combination = CombinationsByTitle[i];
					break;
				}
			}
		}
	}

	for (int palette = 0; palette < 3; ++palette)
	{
		auto& ram = palette == 2 ? m_Background : m_Objects;
		int base = palette == 1 ? 8 : 0;
		for (int colour = 0; colour < 4; ++colour)
		{
			uint16_t rgb = Colours[Combinations[combination][palette] + colour];
			ram[base + colour * 2] = uint8_t(rgb);
			ram[base + colour * 2 + 1] = uint8_t(rgb >> 8);
		}
	}
}
