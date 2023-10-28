#pragma once

#include <cstdint>

struct EmulatorContext;

const uint8_t ReadFromBus(EmulatorContext* context, const uint16_t address);
void WriteToBus(EmulatorContext* context, uint16_t address, uint8_t data);

class IBus
{
public:
	IBus() = default;
	virtual ~IBus() = default;

	virtual uint8_t ReadBus(uint16_t address) = 0;
	virtual void WriteBus(uint16_t address, uint8_t value) = 0;
};