#include "Pch.h"
#include "Ppu.h"
#include "Cpu.h"
#include "Display.h"
#include "Emulator.h"
#include "Cartridge.h"
#include "HighTimer.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <Dma.h>

Ppu::Ppu()
{
	m_Cpu = Emulator::Instance->GetCpu();
	m_Display = Emulator::Instance->GetDisplay();
	m_Cartridge = Emulator::Instance->GetCartridge();
}

Ppu::Ppu(Cpu* cpu, Display* display, Cartridge* cartridge) : m_Cpu(cpu), m_Display(display), m_Cartridge(cartridge)
{
}

void Ppu::Init()
{
	m_PixelProcessor = std::make_unique<PixelProcessor>(m_Display, m_Cpu, m_Cartridge);
	m_Pipeline = std::make_unique<Pipeline>(this, m_Display, m_Cartridge);

	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);

	m_Timer.Start();
}

void Ppu::Tick()
{  
	if (!m_Display->IsLcdEnabled())
	{
		return;
	}

	const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots++;

	// Display mode
	switch (m_Display->GetLcdMode())
	{
		case LcdMode::OAM:
			UpdateOam();
			// m_PixelProcessor->UpdateOam();
			break;
		case LcdMode::PixelTransfer:
			PixelTransfer();
			break;
		case LcdMode::HBlank:
			HBlank();
			//m_PixelProcessor->UpdateHBlank();
			break;
		case LcdMode::VBlank:
			VBlank();
			// m_PixelProcessor->UpdateVBlank();
			break;
	}
}

void Ppu::WriteOam(uint16_t address, uint8_t value)
{
	m_PixelProcessor->WriteOam(address, value);
}

uint8_t Ppu::ReadOam(uint16_t address)
{
	return m_PixelProcessor->ReadOam(address);
}

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	m_PixelProcessor->WriteVideoRam(address, value);
}

uint8_t Ppu::ReadVideoRam(uint16_t address)
{
	return m_PixelProcessor->ReadVideoRam(address);
}

uint8_t Ppu::ReadVideoRam(uint16_t address, uint8_t bank)
{
	return m_PixelProcessor->PipelineReadVideoRam(address, bank);
}

void Ppu::SetVideoRamBank(uint8_t value)
{
	m_PixelProcessor->SetVideoRamBank(value);
}

void Ppu::UpdateOam()
{
	// TODO: Temp for migrating to PixelProcessor class
	PixelProcessorContext* context = const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext());

	// Searching for objects takes 80 ticks
	if (const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
	}

	// Search and order OAMA per line
	if (const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots == 1)
	{
		context->objects_per_line.clear();

		// Find all objects on the current scan line
		uint8_t sprite_height = m_Display->GetObjectHeight();
		for (auto& oam : context->oam_ram)
		{
			if ((oam.position_y + sprite_height > m_Display->m_Context.ly + 16) && (oam.position_y <= m_Display->m_Context.ly + 16))
			{
				context->objects_per_line.push_back(oam);
			}
		}

		// Sort by priority and X position
		if (m_Cartridge->IsColourModeDMG() || m_Display->IsObjectPriorityModeSet())
		{
			std::sort(context->objects_per_line.begin(), context->objects_per_line.end(), [](const OamDataV2& lhs, const OamDataV2& rhs)
			{
				return (lhs.position_x < rhs.position_x);
			});
		}

		// Limit to 10 per row
		if (context->objects_per_line.size() > 10)
		{
			context->objects_per_line.erase(context->objects_per_line.begin() + 10, context->objects_per_line.end());
		}
	}
}

void Ppu::PixelTransfer()
{
	m_Pipeline->PipelineProcess();

	// Process pixels until we finish the line
	if (m_Pipeline->GetContext()->pushed_x >= ScreenResolutionX)
	{
		m_Pipeline = std::make_unique<Pipeline>(this, m_Display, m_Cartridge);
		m_Display->SetLcdMode(LcdMode::HBlank);
	}
}

void Ppu::VBlank()
{
	if (m_Display->m_Context.ly == 153 && const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots == 4)
	{
		m_Display->m_Context.ly = 0;
		CheckLYCFlag();
	}

	if (const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots >= m_DotTicksPerLine)
	{
		// Keep increasing LY register until we reach the lines per frame
		if (m_Display->m_Context.ly == 0)
		{
			const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots = 0;
			IncrementLY();

			LimitFrameRate();

			m_Display->SetLcdMode(LcdMode::OAM);
			m_Display->m_Context.ly = 0;
			const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->window_line = 0;
		}
		else
		{
			const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots = 0;
			IncrementLY();
		}
	}
}

void Ppu::HBlank()
{
	if (const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots >= m_DotTicksPerLine)
	{
		const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->dots = 0;
		IncrementLY();

		// Enter VBlank if all the scanlines have been drawn
		if (m_Display->m_Context.ly >= ScreenResolutionY)
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

bool Ppu::IsWindowVisible()
{
	return m_Display->IsWindowEnabled()
		&& m_Display->m_Context.wx >= 0
		&& m_Display->m_Context.wx <= 166
		&& m_Display->m_Context.wy >= 0
		&& m_Display->m_Context.wy < ScreenResolutionY;
}

void Ppu::IncrementLY()
{
	// Increment LY register
	m_Display->m_Context.ly++;

	// Internal window line is used for the window tiles Y offset and only incremented when the window is visible
	if (IsWindowVisible() && (m_Display->m_Context.ly > m_Display->m_Context.wy) && (m_Display->m_Context.ly <= m_Display->m_Context.wy + ScreenResolutionY))
	{
		const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext())->window_line++;
	}

	CheckLYCFlag();
}

void Ppu::CheckLYCFlag()
{
	if (m_Display->m_Context.ly == m_Display->m_Context.lyc)
	{
		m_Display->m_Context.stat |= 0b100;
		if (m_Display->IsStatInterruptLYC())
		{
			m_Cpu->RequestInterrupt(InterruptFlag::STAT);
		}
	}
	else
	{
		m_Display->m_Context.stat &= ~0b100;
	}
}

void Ppu::LimitFrameRate()
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

void Ppu::SaveState(std::fstream* file)
{
	/*size_t videoram_size = m_Context.video_ram.size();
	file->write(reinterpret_cast<const char*>(&videoram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));*/

	//file->write(reinterpret_cast<const char*>(&m_VramBank), sizeof(m_VramBank));
}

void Ppu::LoadState(std::fstream* file)
{
	/*size_t videoram_size = 0;
	file->read(reinterpret_cast<char*>(&videoram_size), sizeof(size_t));*/

	/*m_Context.video_ram.resize(videoram_size);
	file->read(reinterpret_cast<char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));*/

	//file->read(reinterpret_cast<char*>(&m_VramBank), sizeof(m_VramBank));
}