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
	m_Display = Emulator::Instance->GetDisplay();
}  

void Ppu::Init()
{


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
		case LcdMode::VBlank:
			VBlank();
			break;
		case LcdMode::HBlank:
			HBlank();
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
}

void Ppu::PixelTransfer()
{
}

void Ppu::VBlank()
{
}

void Ppu::HBlank()
{

}