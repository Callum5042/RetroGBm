#pragma once

#include "LengthCounter.h"
#include <cstdint>
#include <memory>

class SoundModeBase
{
public:
	SoundModeBase(int offset, int length);
	virtual ~SoundModeBase();

	virtual int TickChannel() = 0;
	virtual void Trigger() = 0;

	virtual void Write(uint16_t address, uint8_t value);
	virtual int Read(uint16_t address);

	virtual void Start() = 0;
	void Stop();

	bool IsEnabled() const { return ChannelEnabled && DacEnabled; }

protected:
	std::unique_ptr<LengthCounter> Length = nullptr;

	bool DacEnabled = false;
	bool ChannelEnabled = false;

	void UpdateLength();
};