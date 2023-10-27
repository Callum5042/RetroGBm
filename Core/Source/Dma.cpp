#include "Dma.h"
#include "Ppu.h"
#include "Emulator.h"

void Dma::Start(uint8_t start)
{
	context.active = true;
	context.byte = 0;
	context.start_delay = 2;
	context.value = start;
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

	Emulator::Instance->GetPpu()->WriteOam(context.byte, Emulator::Instance->ReadBus((context.value * 0x100) + context.byte));

	context.byte++;
	context.active = context.byte < 0xA0;
}

bool Dma::IsTransferring()
{
	return context.active;
}