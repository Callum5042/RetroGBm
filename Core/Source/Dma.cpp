#include "RetroGBm/Pch.h"
#include "RetroGBm/Dma.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Ppu.h"

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
		m_LengthCode = value & 0x7F;
		m_Length = (m_LengthCode + 1) << 4;
		m_HBlankMode = (value & 0x80) != 0;
		m_Active = true;
	}
	else
	{
		if ((value & 0x80) == 0)
		{
			m_LengthCode = 0x7F;
			m_Length = 0;
			m_HBlankMode = false;
			m_Active = false;
		}
	}

	this->RunGDMA(previous_active);
}

void Dma::Reset()
{
	m_Source = 0;
	m_Destination = 0;
	m_HBlankMode = false;
	m_LengthCode = 0x7F;
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

	m_Ppu->WriteOam(context.byte, m_Bus->ReadBus((context.value * 0x100) + context.byte));

	context.byte++;
	context.active = context.byte < 0xA0;
}

void Dma::RunHDMA()
{
	if (m_Active && m_HBlankMode)
	{
		if (m_Length <= 0)
		{
			Reset();
		}
		else
		{
			for (int i = 0; i < 16; i++)
			{
				uint8_t data = m_Bus->ReadBus(m_DmaSrc++);
				m_Ppu->WriteVideoRam(0x8000 + m_DmaDest++, data);
			}

			m_Length -= 16;
		}
	}
}

void Dma::RunGDMA(bool previous_active)
{
	if (m_Active && !previous_active)
	{
		m_DmaSrc = m_Source;
		m_DmaDest = m_Destination;

		// Peform a general purpose DMA right now
		if (!m_HBlankMode)
		{
			for (int i = 0; i < m_Length; i++)
			{
				uint8_t data = m_Bus->ReadBus(m_DmaSrc++);
				m_Ppu->WriteVideoRam(0x8000 + m_DmaDest++, data);
			}

			Reset();
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

	return 0xFF;
}

void Dma::SaveState(std::fstream* file)
{
	/*file->write(reinterpret_cast<const char*>(&m_ColourDMA), sizeof(m_ColourDMA));
	file->write(reinterpret_cast<const char*>(&m_Source), sizeof(m_Source));
	file->write(reinterpret_cast<const char*>(&m_Destination), sizeof(m_Destination));

	file->write(reinterpret_cast<const char*>(&m_GeneralPurposeDMA), sizeof(m_GeneralPurposeDMA));
	file->write(reinterpret_cast<const char*>(&m_TransferLength), sizeof(m_TransferLength));
	file->write(reinterpret_cast<const char*>(&m_LengthModeStart), sizeof(m_LengthModeStart));*/
}

void Dma::LoadState(std::fstream* file)
{
	/*file->read(reinterpret_cast<char*>(&m_ColourDMA), sizeof(m_ColourDMA));
	file->read(reinterpret_cast<char*>(&m_Source), sizeof(m_Source));
	file->read(reinterpret_cast<char*>(&m_Destination), sizeof(m_Destination));

	file->read(reinterpret_cast<char*>(&m_GeneralPurposeDMA), sizeof(m_GeneralPurposeDMA));
	file->read(reinterpret_cast<char*>(&m_TransferLength), sizeof(m_TransferLength));
	file->read(reinterpret_cast<char*>(&m_LengthModeStart), sizeof(m_LengthModeStart));*/
}
