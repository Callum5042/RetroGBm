#include "Pch.h"
#include "Dma.h"
#include "Ppu.h"
#include "Emulator.h"

Dma::Dma()
{
	m_Bus = Emulator::Instance;
	m_Ppu = Emulator::Instance->GetPpu();
}

void Dma::Start(uint8_t start)
{
	m_ColourDMA = false;
	context.active = true;
	context.byte = 0;
	context.start_delay = 2;
	context.value = start;
}

void Dma::StartCGB(uint8_t value)
{
	m_ColourDMA = true;
	m_LengthModeStart = value;

	// General-Purpose DMA if 0 otherwise do HBlank DMA
	m_GeneralPurposeDMA = (value & 0x80) != 0x80;

	m_TransferLength = ((value & 0x7F) + 1) * 10;
}

void Dma::Tick()
{
	if (m_ColourDMA)
	{
		if (m_GeneralPurposeDMA)
		{
			for (int tick = 0; tick < m_TransferLength; ++tick)
			{
				uint16_t source = m_Source & 0xFFF0;
				uint16_t destination = m_Destination & 0x1FF0;

				uint8_t value = m_Bus->ReadBus(source + tick);
				m_Ppu->WriteOam(destination + tick, value);
			}
		}
	}
	else
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
}

bool Dma::IsTransferring()
{
	return context.active;
}

void Dma::SetSource(uint16_t address, uint8_t value)
{
	if (address == 0xFF51)
	{
		m_Source |= (value << 8);
	}
	else if (address == 0xFF52)
	{
		m_Source |= value;
	}
}

void Dma::SetDestination(uint16_t address, uint8_t value)
{
	if (address == 0xFF53)
	{
		m_Destination |= (value << 8);
	}
	else if (address == 0xFF54)
	{
		m_Destination |= value;
	}
}