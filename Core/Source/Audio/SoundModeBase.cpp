#include "RetroGBm/Pch.h"
#include "RetroGBm/Audio/SoundModeBase.h"

SoundModeBase::SoundModeBase(int offset, int length)
{
	Length = std::make_unique<LengthCounter>(length);
}

SoundModeBase::~SoundModeBase()
{
}

void SoundModeBase::Write(uint16_t address, uint8_t value)
{
}

int SoundModeBase::Read(uint16_t address)
{
	return 0xFF;
}

void SoundModeBase::Stop()
{
	ChannelEnabled = false;
}

void SoundModeBase::UpdateLength()
{
	Length->Tick();
	if (Length->IsEnabled() && Length->GetLength() == 0)
	{
		ChannelEnabled = false;
	}
}
