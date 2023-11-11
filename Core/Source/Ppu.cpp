#include "Pch.h"
#include "Ppu.h"
#include "Cpu.h"
#include "Display.h"
#include "Emulator.h"
#include <algorithm>
#include <chrono>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Ppu::Ppu()
{
	m_Bus = Emulator::Instance;
	m_Cpu = Emulator::Instance->GetCpu();
	m_Display = Emulator::Instance->GetDisplay();
}  

void Ppu::Init()
{
	m_Context.video_buffer.resize(0x8000);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0);

	m_Context.video_buffer.resize(ScreenResolutionX * ScreenResolutionY * sizeof(uint32_t));
	std::fill(m_Context.video_buffer.begin(), m_Context.video_buffer.end(), 0);



	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);
}

void Ppu::Tick()
{
	m_Context.dot_ticks++;

	switch (m_Display->GetLcdMode())
	{
		case LcdMode::OAM:
			UpdateOam();
			break;
		case LcdMode::PixelTransfer:
			PixelTransfer();
			break;
		case LcdMode::HBlank:
			HBlank();
			break;
		case LcdMode::VBlank:
			VBlank();
			break;
	}
}

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	m_Context.video_ram[address - 0x8000] = value;
}

uint8_t Ppu::ReadVideoRam(uint16_t address)
{
	return m_Context.video_ram[address - 0x8000];
}

void Ppu::UpdateOam()
{
	// OAM takes 80 ticks
	if (m_Context.dot_ticks >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
	}
}

void Ppu::PixelTransfer()
{
	m_PixelX++;

	if (m_PixelX > ScreenResolutionX)
	{
		m_Display->context.ly++;

		m_Display->SetLcdMode(LcdMode::HBlank);
	}
}

void Ppu::HBlank()
{
	// Each line takes 456 ticks
	if (m_Context.dot_ticks >= 456)
	{
		m_Context.dot_ticks = 0;
		if (m_Display->context.ly < 144)
		{
			m_Display->SetLcdMode(LcdMode::OAM);
		}
		else
		{
			m_Cpu->RequestInterrupt(InterruptFlag::VBlank);
			m_Display->SetLcdMode(LcdMode::VBlank);
		}
	}
}

void Ppu::VBlank()
{
	if (m_Context.dot_ticks >= 456)
	{
		m_Context.dot_ticks = 0;
		m_Display->context.ly++;

		// 153 total lines in a frame (144 on screen, 10 extra for VBlank mode) (4560 dot ticks)
		if (m_Display->context.ly >= 154)
		{
			m_Display->context.ly = 0;
			m_Display->SetLcdMode(LcdMode::OAM);
		}
	}
}

bool Ppu::CanAccessVRam()
{
	LcdMode mode = m_Display->GetLcdMode();
	return (mode != LcdMode::PixelTransfer);
}

bool Ppu::CanAccessOAM()
{
	LcdMode mode = m_Display->GetLcdMode();
	if (mode == LcdMode::OAM || mode == LcdMode::PixelTransfer)
	{
		return false;
	}

	return true;
}