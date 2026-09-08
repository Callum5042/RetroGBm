#include "RetroGBm/Pch.h"
#include "RetroGBm/Dma.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/VideoState.h"

Dma::Dma()
{
	m_Bus = Emulator::Instance;
	m_Ppu = Emulator::Instance->GetPpu();

	Reset();
}

void Dma::Start(uint8_t start)
{
	context.active = true;
	context.byte = 0;
	context.start_delay = 2;
	context.value = start;
}

void Dma::StartCGB(uint8_t value)
{
	bool previous_active = m_Active;

	if (!m_Active)
	{
		m_LengthStart = value & 0x7F;
		m_Length = (m_LengthStart + 1) << 4;
		m_HBlankMode = (value & 0x80) != 0;
		m_Active = true;
	}
	else
	{
		if ((value & 0x80) == 0)
		{
			m_HBlankMode = false;
			m_Active = false;
		}
	}

	this->RunGDMA(previous_active);
}

void Dma::Reset()
{
	context = {};
	m_Length = 0;
	m_StallCycles = 0;
	m_Source = 0;
	m_Destination = 0;
	m_HBlankMode = false;
	m_LengthStart = 0x7F;
	m_Active = false;

	m_DmaSrc = 0;
	m_DmaDest = 0;
}

void Dma::Tick()
{
	if (!context.active)
	{
		return;
	}

	if (context.start_delay)
	{
		context.start_delay--;
		return;
	}

	m_Ppu->WriteOamDma(context.byte, m_Bus->ReadBus((context.value * 0x100) + context.byte));

	context.byte++;
	context.active = context.byte < 0xA0;
}

void Dma::RunHDMA()
{
	if (m_Active && m_HBlankMode)
	{
		TransferBlock();
	}
}

void Dma::TransferBlock()
{
	for (int i = 0; i < 16; ++i)
	{
		m_Ppu->WriteVideoRamDma(0x8000 | (m_DmaDest & 0x1FFF), m_Bus->ReadBus(m_DmaSrc++));
		m_DmaDest = (m_DmaDest + 1) & 0x1FFF;
	}

	m_Length -= 16;
	m_Source = m_DmaSrc;
	m_Destination = m_DmaDest;
	m_StallCycles += Emulator::Instance->IsDoubleSpeedMode() ? 16 : 8;

	if (!m_Length)
	{
		m_Active = false;
	}
}

void Dma::RunGDMA(bool previous_active)
{
	if (m_Active && !previous_active)
	{
		m_DmaSrc = m_Source;
		m_DmaDest = m_Destination;

		// Peform a general purpose DMA right now
		if (!m_HBlankMode || !Emulator::Instance->GetDisplay()->IsLcdEnabled())
		{
			while (m_Active) TransferBlock();
		}
		else if (Emulator::Instance->GetDisplay()->GetLcdMode() == LcdMode::HBlank && Emulator::Instance->GetDisplay()->Read(0xFF44) < 144)
		{
			RunHDMA();
		}
	}
}

bool Dma::IsTransferring() const
{
	return context.active;
}

void Dma::SetSource(uint16_t address, uint8_t value)
{
	if (address == 0xFF51)
	{
		m_Source = (value << 8) | (m_Source & 0xFF);
	}
	else if (address == 0xFF52)
	{
		m_Source = (m_Source & 0xFF00) | (value & 0xF0);
	}
}

void Dma::SetDestination(uint16_t address, uint8_t value)
{
	if (address == 0xFF53)
	{
		m_Destination = ((value & 0x1F) << 8) | (m_Destination & 0xFF);
	}
	else if (address == 0xFF54)
	{
		m_Destination = (m_Destination & 0xFF00) | (value & 0xF0);
	}
}

uint8_t Dma::GetHDMA5() const
{
	if (m_Active)
	{
		return (m_Length / 16) - 1;
	}

	return m_Length ? uint8_t(0x80 | ((m_Length / 16) - 1)) : 0xFF;
}

void Dma::SaveState(std::fstream* file)
{
	VideoState::Write(*file, context.active);
	VideoState::Write(*file, context.byte);
	VideoState::Write(*file, context.value);
	VideoState::Write(*file, context.start_delay);
	VideoState::Write(*file, int32_t(m_StallCycles));
	file->write(reinterpret_cast<const char*>(&m_Source), sizeof(m_Source));
	file->write(reinterpret_cast<const char*>(&m_Destination), sizeof(m_Destination));

	file->write(reinterpret_cast<const char*>(&m_Active), sizeof(m_Active));
	file->write(reinterpret_cast<const char*>(&m_HBlankMode), sizeof(m_HBlankMode));
	file->write(reinterpret_cast<const char*>(&m_Length), sizeof(m_Length));
	file->write(reinterpret_cast<const char*>(&m_LengthStart), sizeof(m_LengthStart));

	file->write(reinterpret_cast<const char*>(&m_DmaSrc), sizeof(m_DmaSrc));
	file->write(reinterpret_cast<const char*>(&m_DmaDest), sizeof(m_DmaDest));
}

void Dma::LoadState(std::fstream* file)
{
	VideoState::Read(*file, context.active);
	VideoState::Read(*file, context.byte);
	VideoState::Read(*file, context.value);
	VideoState::Read(*file, context.start_delay);
	int32_t stalls;
	VideoState::Read(*file, stalls);
	if (stalls < 0 || stalls > 2048) throw std::runtime_error("Invalid DMA state");
	m_StallCycles = stalls;
	file->read(reinterpret_cast<char*>(&m_Source), sizeof(m_Source));
	file->read(reinterpret_cast<char*>(&m_Destination), sizeof(m_Destination));

	file->read(reinterpret_cast<char*>(&m_Active), sizeof(m_Active));
	file->read(reinterpret_cast<char*>(&m_HBlankMode), sizeof(m_HBlankMode));
	file->read(reinterpret_cast<char*>(&m_Length), sizeof(m_Length));
	file->read(reinterpret_cast<char*>(&m_LengthStart), sizeof(m_LengthStart));

	file->read(reinterpret_cast<char*>(&m_DmaSrc), sizeof(m_DmaSrc));
	file->read(reinterpret_cast<char*>(&m_DmaDest), sizeof(m_DmaDest));
}
