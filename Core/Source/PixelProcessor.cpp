#include "Pch.h"
#include "PixelProcessor.h"
#include "Display.h"
#include "Cpu.h"
#include "Cartridge.h"
#include "PixelPipeline.h"

#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <memory>

PixelProcessor::PixelProcessor(Display* display) : m_Display(display)
{
	Init();
}

PixelProcessor::PixelProcessor(Display* display, Cpu* cpu) : m_Display(display), m_Cpu(cpu)
{
	Init();
}

PixelProcessor::PixelProcessor(Display* display, Cpu* cpu, Cartridge* cartridge) : m_Display(display), m_Cpu(cpu), m_Cartridge(cartridge)
{
	Init();
}

void PixelProcessor::Init()
{
	m_Pipeline = std::make_unique<PixelPipeline>(this, m_Display, m_Cartridge);

	// 16Kb of VRAM (2 banks of 8Kb each) and fills it to default value of 0
	m_Context.video_ram.resize(0x4000);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0x0);
}

void PixelProcessor::Tick()
{
	if (!m_Display->IsLcdEnabled())
	{
		return;
	}

	// Increase dots every tick of the PPU (PPU is ticked 1 times per T-cycle on single speed mode and 2 times per T-cycle on double speed mode)
	m_Context.dots++;

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
	//if (m_Display->IsLcdEnabled())
	//{
	//	// TODO: Check values https://gbdev.io/pandocs/pixel_fifo.html#vram-access
	//	throw std::exception("TODO");
	//}

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

void PixelProcessor::WriteOam(uint16_t address, uint8_t value)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* ptr = reinterpret_cast<uint8_t*>(m_Context.oam_ram.data());
	ptr[address] = value;
}

uint8_t PixelProcessor::ReadOam(uint16_t address)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* ptr = reinterpret_cast<uint8_t*>(m_Context.oam_ram.data());
	return ptr[address];
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

void PixelProcessor::CheckLYCFlag()
{
	DisplayContext* display_context = const_cast<DisplayContext*>(m_Display->GetContext());
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
}

void PixelProcessor::UpdateOam()
{
	// Searching for objects takes 80 ticks
	if (m_Context.dots >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);

		return;
	}

	// Search and order OAMA per line
	if (m_Context.dots == 1)
	{
		m_Context.objects_per_line.clear();

		// Find all objects on the current scan line
		uint8_t sprite_height = m_Display->GetObjectHeight();
		for (auto& oam : m_Context.oam_ram)
		{
			const DisplayContext* display_context = m_Display->GetContext();
			if ((oam.position_y + sprite_height > display_context->ly + 16) && (oam.position_y <= display_context->ly + 16))
			{
				m_Context.objects_per_line.push_back(oam);
			}
		}

		// Sort by priority and X position
		if (m_Cartridge->IsColourModeDMG() || m_Display->IsObjectPriorityModeSet())
		{
			std::sort(m_Context.objects_per_line.begin(), m_Context.objects_per_line.end(), [](const OamDataV2& lhs, const OamDataV2& rhs)
			{
				return (lhs.position_x < rhs.position_x);
			});
		}

		// Limit to 10 per row
		if (m_Context.objects_per_line.size() > 10)
		{
			m_Context.objects_per_line.erase(m_Context.objects_per_line.begin() + 10, m_Context.objects_per_line.end());
		}
	}
}

void PixelProcessor::UpdatePixelTransfer()
{
	m_Pipeline->PipelineProcess();

	// Process pixels until we finish the line
	if (m_Pipeline->GetContext()->pushed_x >= ScreenResolutionX)
	{
		m_Pipeline = std::make_unique<PixelPipeline>(this, m_Display, m_Cartridge);
		m_Display->SetLcdMode(LcdMode::HBlank);
	}
}

void PixelProcessor::UpdateHBlank()
{
	int dot_duration = 456;
	if (m_Context.dots >= dot_duration)
	{
		m_Context.dots = 0;
		
		// Update LY
		m_Display->GetContext()->ly += 1;
		CheckLYCFlag();

		// Update window line only if window is enabled
		const DisplayContext* display_context = m_Display->GetContext();
		if (m_Display->IsWindowVisible() && (display_context->ly > display_context->wy) && (display_context->ly <= display_context->wy + ScreenResolutionY))
		{
			m_Context.window_line += 1;
		}

		// Enter VBlank if all the scanlines have been drawn
		if (display_context->ly >= ScreenResolutionY)
		{
			m_Display->SetLcdMode(LcdMode::VBlank);

			m_Cpu->RequestInterrupt(InterruptFlag::VBlank);
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
	DisplayContext* display_context = m_Display->GetContext();
	if (display_context->ly == 153 && m_Context.dots == 4)
	{
		display_context->ly = 0;
		CheckLYCFlag();
	}

	int dots_per_line = 456;
	if (m_Context.dots >= dots_per_line)
	{
		m_Context.dots = 0;

		// Keep increasing LY register until we reach the lines per frame
		if (display_context->ly == 0)
		{
			m_Display->SetLcdMode(LcdMode::OAM);
			display_context->ly = 0;
			m_Context.window_line = 0;

			// TODO: This really feels like it shouldn't belong here?
			LimitFrameRate();
		}
		else
		{
			IncrementLY();
		}
	}
}

void PixelProcessor::IncrementLY()
{
	DisplayContext* display_context = m_Display->GetContext();

	// Increment LY register
	display_context->ly++;

	// Internal window line is used for the window tiles Y offset and only incremented when the window is visible
	if (m_Display->IsWindowVisible() && (display_context->ly > display_context->wy) && (display_context->ly <= display_context->wy + ScreenResolutionY))
	{
		m_Context.window_line++;
	}

	// Check flag
	CheckLYCFlag();
}

void PixelProcessor::LimitFrameRate()
{
	m_Timer.Tick();
	m_FrameCount++;

	// Compute averages over one second period
	if ((m_Timer.TotalTime() - m_TimeElapsed) >= 1.0f)
	{
		m_FramesPerSecond = m_FrameCount;
		float mspf = 1000.0f / m_FramesPerSecond;

		// Set total frame count
		m_TotalFrames += m_FrameCount;

		// Reset for next average.
		m_FrameCount = 0;
		m_TimeElapsed += 1.0f;
	}

	// Limit framerate to match target rate
	if (m_Timer.DeltaTime() < m_TargetFrameTime)
	{
#ifdef _WIN32
		m_Timer.Stop();
		const std::chrono::duration<double, std::milli> elapsed(m_TargetFrameTime - m_Timer.DeltaTime());
		std::this_thread::sleep_for(elapsed);
		m_Timer.Start();
#else
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(16ms);
#endif
	}
}