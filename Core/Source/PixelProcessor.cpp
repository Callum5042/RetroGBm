#include "Pch.h"
#include "PixelProcessor.h"
#include "Display.h"
#include "Cpu.h"

#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <iomanip>

PixelProcessor::PixelProcessor(Display* display) : m_Display(display)
{
	Init();
}

PixelProcessor::PixelProcessor(Display* display, Cpu* cpu) : m_Display(display), m_Cpu(cpu)
{
	Init();
}

void PixelProcessor::Init()
{
	// 16Kb of VRAM (2 banks of 8Kb each) and fills it to default value of 0
	m_Context.video_ram.resize(0x4000);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0x0);
}

void PixelProcessor::Tick()
{
	// Increase dots every tick of the PPU (PPU is ticked 1 times per T-cycle on single speed mode and 2 times per T-cycle on double speed mode)
	m_Context.dots++;
	m_Context.frame_dots++;

	// Check LY==LYC registers
	DisplayContext* display_context = m_Display->GetContext();
	if (display_context->ly == display_context->lyc)
	{
		display_context->stat |= 0b100;
		if (m_Display->IsStatInterruptLYC())
		{
			m_Cpu->RequestInterrupt(InterruptFlag::STAT);
		}
	}
	else
	{
		display_context->stat &= ~0b100;
	}

	// Rendering mode
	LcdMode mode = m_Display->GetLcdMode();
	switch (mode)
	{
		case LcdMode::OAM:
			UpdateOam();
			break;
		case LcdMode::PixelTransfer:
			UpdatePixelTransfer();
			break;
		case LcdMode::HBlank:
			UpdateHBlank();
			break;
		case LcdMode::VBlank:
			UpdateVBlank();
			break;
	}
}

void PixelProcessor::WriteVideoRam(uint16_t address, uint8_t value)
{
	// Assertion
	if (address < 0x8000 || address > 0x9FFF)
	{
		std::stringstream ss;
		ss << "Invalid VRAM address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec;
		throw std::runtime_error(ss.str());
	}

	// Ignore writes to VRAM if inaccessible
	if (!IsVideoRamAccessable())
	{
		return;
	}

	// Calculate address
	address = (address - 0x8000) + (m_Context.video_ram_bank * 0x2000);

	// Assert bank ranges
#ifdef _DEBUG
	if (m_Context.video_ram_bank == 0 && address >= 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << m_Context.video_ram_bank;
		throw std::runtime_error(ss.str());
	}

	if (m_Context.video_ram_bank == 1 && address < 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << m_Context.video_ram_bank;
		throw std::runtime_error(ss.str());
	}
#endif

	// Writes to address
	m_Context.video_ram[address] = value;
}

uint8_t PixelProcessor::ReadVideoRam(uint16_t address)
{
	// Assertion
	if (address < 0x8000 || address > 0x9FFF)
	{
		std::stringstream ss;
		ss << "Invalid VRAM address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec;
		throw std::runtime_error(ss.str());
	}

	// Read garabage (0xFF) from VRAM if inaccessible
	if (!IsVideoRamAccessable())
	{
		return 0xFF;
	}

	// Calculate address
	address = (address - 0x8000) + (m_Context.video_ram_bank * 0x2000);

	// Assert bank ranges
#ifdef _DEBUG
	if (m_Context.video_ram_bank == 0 && address >= 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << m_Context.video_ram_bank;
		throw std::runtime_error(ss.str());
	}

	if (m_Context.video_ram_bank == 1 && address < 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << m_Context.video_ram_bank;
		throw std::runtime_error(ss.str());
	}
#endif

	return m_Context.video_ram[address];
}

uint8_t PixelProcessor::PipelineReadVideoRam(uint16_t address, int bank)
{
	// Assertion
	if (address < 0x8000 || address > 0x9FFF)
	{
		std::stringstream ss;
		ss << "Invalid VRAM address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec;
		throw std::runtime_error(ss.str());
	}

	// Check if access is blocked
	if (m_Display->IsLcdEnabled())
	{
		// TODO: Check values https://gbdev.io/pandocs/pixel_fifo.html#vram-access
		throw std::exception("TODO");
	}

	// Calculate address
	address = (address - 0x8000) + (bank * 0x2000);

	// Assert bank ranges
#ifdef _DEBUG
	if (bank == 0 && address >= 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << bank;
		throw std::runtime_error(ss.str());
	}

	if (bank == 1 && address < 0x2000)
	{
		std::stringstream ss;
		ss << "Invalid VRAM banked address 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)address << std::dec
			<< " - bank register 0x" << bank;
		throw std::runtime_error(ss.str());
	}
#endif

	return m_Context.video_ram[address];
}

bool PixelProcessor::IsVideoRamAccessable()
{
	// VRAM always accessable if display is off
	if (!m_Display->IsLcdEnabled())
	{
		return true;
	}

	// Only PixelTransfer (mode 3) disables access to VRAM
	LcdMode mode = m_Display->GetLcdMode();
	switch (mode)
	{
		case LcdMode::HBlank:
		case LcdMode::VBlank:
		case LcdMode::OAM:
			return true;
		case LcdMode::PixelTransfer:
			return false;
		default:
			throw std::runtime_error("Unknown LcdMode in IsVideoRamAccessable");
	}
}

uint8_t PixelProcessor::GetVideoRamBank() const
{
	// Bits 7-1 are always 1 by default (pattern: 0b1111_111x)
	return (0b1111'1110 | m_Context.video_ram_bank);
}

void PixelProcessor::SetVideoRamBank(uint8_t value)
{
	// Only bit 0 matters
	m_Context.video_ram_bank = value & 0b1;
}

void PixelProcessor::UpdateOam()
{
	// Searching for objects takes 80 ticks
	if (m_Context.dots >= 80)
	{
		// m_Context.pipeline = PipelineContext();
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
		m_Context.dots = 0;

		return;
	}

	// Search and order OAMA per line
	//if (m_Context.dots == 1)
	//{
	//	m_Context.objects_per_line.clear();

	//	// Find all objects on the current scan line
	//	uint8_t sprite_height = m_Display->GetObjectHeight();
	//	for (auto& oam : m_Context.oam_ram)
	//	{
	//		if ((oam.position_y + sprite_height > m_Display->m_Context.ly + 16) && (oam.position_y <= m_Display->m_Context.ly + 16))
	//		{
	//			m_Context.objects_per_line.push_back(oam);
	//		}
	//	}

	//	// Sort by priority and X position
	//	if (m_Cartridge->IsColourModeDMG() || m_Display->IsObjectPriorityModeSet())
	//	{
	//		std::sort(m_Context.objects_per_line.begin(), m_Context.objects_per_line.end(), [](const OamData& lhs, const OamData& rhs)
	//		{
	//			return (lhs.position_x < rhs.position_x);
	//		});
	//	}

	//	// Limit to 10 per row
	//	if (m_Context.objects_per_line.size() > 10)
	//	{
	//		m_Context.objects_per_line.erase(m_Context.objects_per_line.begin() + 10, m_Context.objects_per_line.end());
	//	}
	//}
}

void PixelProcessor::UpdatePixelTransfer()
{

}

void PixelProcessor::UpdateHBlank()
{
	int dot_duration = 456;
	if ((m_Context.frame_dots % dot_duration) == 0)
	{
		m_Context.dots = 0;

		// Update LY
		m_Display->GetContext()->ly += 1;

		// Update window line only if window is enabled
		const DisplayContext* display_context = m_Display->GetContext();
		if (m_Display->IsWindowVisible() && (display_context->ly > display_context->wy) && (display_context->ly <= display_context->wy + ScreenResolutionY))
		{
			m_Context.window_line += 1;
		}

		// Set LCD Mode VBlank if we are at end of the frame (Line 144)
		if (m_Display->GetContext()->ly == 144)
		{
			m_Display->SetLcdMode(LcdMode::VBlank);

			if (m_Display->IsStatInterruptVBlank())
			{
				m_Cpu->RequestInterrupt(InterruptFlag::VBlank);
			}
		}
		else
		{
			m_Display->SetLcdMode(LcdMode::OAM);
		}
	}
}

void PixelProcessor::UpdateVBlank()
{
	// Update LY for VBlank
	if ((m_Context.dots % 456) == 0)
	{
		m_Display->GetContext()->ly += 1;

		// TODO: Not actually sure if we want to increase window_line in VBlank
		const DisplayContext* display_context = m_Display->GetContext();
		if (m_Display->IsWindowVisible() && (display_context->ly > display_context->wy) && (display_context->ly <= display_context->wy + ScreenResolutionY))
		{
			m_Context.window_line += 1;
		}
	}

	// Check for end of VBlank mode
	int dots_duration = 4560;
	if (m_Context.dots == dots_duration)
	{
		if (m_Display->GetContext()->ly != 154)
		{
			throw std::runtime_error("Attempted to finish VBlank without LY being 154");
		}

		m_Context.dots = 0;
		m_Context.window_line = 0;
		m_Display->GetContext()->ly = 0;
		m_Display->SetLcdMode(LcdMode::OAM);

		return;
	}
}
