#include "RetroGBm/Pch.h"
#include "RetroGBm/Dma.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Ppu.h"

Dma::Dma()
{
	m_Bus = Emulator::Instance;
	m_Ppu = Emulator::Instance->GetPpu();
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
	m_HDMA5 = value;

	if (m_EnableHDMA && m_HBlankDMA)
	{
		if ((value & 0x80) == 0x80)
		{
			// Restart copy
			m_TransferLength = ((value & 0x7F) + 1) * 16;
			return;
		}
		else
		{
			// Stop HDMA if active
			m_HBlankDMA = false;
			m_EnableHDMA = false;
			m_HdmaByte = 0;
			m_TransferLength = value;
			return;
		}
	}

	// General-Purpose DMA if 0 otherwise do HBlank DMA
	m_GeneralPurposeDMA = (value & 0x80) != 0x80;
	m_HBlankDMA = (value & 0x80) == 0x80;

	m_TransferLength = ((value & 0x7F) + 1) * 16;

	if (m_HBlankDMA)
	{
		if (Emulator::Instance->GetDisplay()->GetLcdMode() == LcdMode::HBlank)
		{
			return;
		}

		m_EnableHDMA = true;
		m_HdmaByte = 0;

		// Copy 1 block if the screen is off
		if (!Emulator::Instance->GetDisplay()->IsLcdEnabled())
		{
			for (int i = 0; i < 16; ++i)
			{
				uint8_t source_byte = m_Bus->ReadBus((m_Source & 0xFFF0) + m_HdmaByte);
				m_Ppu->WriteVideoRam(0x8000 + (m_Destination & 0x1FF0) + m_HdmaByte, source_byte);

				m_HdmaByte++;
				m_TransferLength--;
			}
		}
	}

	if (m_GeneralPurposeDMA)
	{
		uint8_t byte = 0;
		while (m_TransferLength > 0)
		{
			uint8_t source_byte = m_Bus->ReadBus((m_Source & 0xFFF0) + byte);
			m_Ppu->WriteVideoRam(0x8000 + (m_Destination & 0x1FF0) + byte, source_byte);

			byte++;
			m_TransferLength--;
		}

		m_EnableHDMA = false;
		m_GeneralPurposeDMA = false;
		m_HBlankDMA = false;
	}
}

void Dma::Tick()
{
	RunHDMA();

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
	if (m_EnableHDMA && m_HBlankDMA)
	{
		if (Emulator::Instance->GetDisplay()->GetLcdMode() == LcdMode::HBlank)
		{
			if (!Emulator::Instance->GetDisplay()->IsLcdEnabled())
			{
				return;
			}

			if (m_ByteBlockTransfered)
			{
				return;
			}

			for (int i = 0; i < 16; ++i)
			{
				uint16_t address = (m_Source & 0xFFF0) + m_HdmaByte;
				uint8_t source_byte = m_Bus->ReadBus(address);
				m_Ppu->WriteVideoRam(0x8000 + (m_Destination & 0x1FF0) + m_HdmaByte, source_byte);

				m_HdmaByte++;
				m_TransferLength--;

				if (m_TransferLength == 0)
				{
					m_EnableHDMA = false;
					m_GeneralPurposeDMA = false;
					m_HBlankDMA = false;
				}
			}

			m_ByteBlockTransfered = true;
		}
		else
		{
			m_ByteBlockTransfered = false;
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
		m_Source &= 0xF0;
		m_Source |= value << 8;

		if (m_Source >= 0xE000)
		{
			m_Source |= 0xF000;
		}
	}
	else if (address == 0xFF52)
	{
		m_Source &= 0xFF00;
		m_Source |= value & 0xF0;
	}
}

void Dma::SetDestination(uint16_t address, uint8_t value)
{
	if (address == 0xFF53)
	{
		m_Destination &= 0xF0;
		m_Destination |= value << 8;
	}
	else if (address == 0xFF54)
	{
		m_Destination &= 0xFF00;
		m_Destination |= value & 0xF0;
	}
}

uint8_t Dma::GetHDMA5() const
{
	if (m_EnableHDMA)
	{
		return (m_TransferLength / 16) - 1;
	}

	return 0xFF;
}

void Dma::SaveState(std::fstream* file)
{
	file->write(reinterpret_cast<const char*>(&m_ColourDMA), sizeof(m_ColourDMA));
	file->write(reinterpret_cast<const char*>(&m_Source), sizeof(m_Source));
	file->write(reinterpret_cast<const char*>(&m_Destination), sizeof(m_Destination));

	file->write(reinterpret_cast<const char*>(&m_GeneralPurposeDMA), sizeof(m_GeneralPurposeDMA));
	file->write(reinterpret_cast<const char*>(&m_TransferLength), sizeof(m_TransferLength));
	file->write(reinterpret_cast<const char*>(&m_LengthModeStart), sizeof(m_LengthModeStart));
}

void Dma::LoadState(std::fstream* file)
{
	file->read(reinterpret_cast<char*>(&m_ColourDMA), sizeof(m_ColourDMA));
	file->read(reinterpret_cast<char*>(&m_Source), sizeof(m_Source));
	file->read(reinterpret_cast<char*>(&m_Destination), sizeof(m_Destination));

	file->read(reinterpret_cast<char*>(&m_GeneralPurposeDMA), sizeof(m_GeneralPurposeDMA));
	file->read(reinterpret_cast<char*>(&m_TransferLength), sizeof(m_TransferLength));
	file->read(reinterpret_cast<char*>(&m_LengthModeStart), sizeof(m_LengthModeStart));
}
