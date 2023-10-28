#pragma once

#include <cstdint>

class IBus;
class Ppu;

struct DmaContext
{
	bool active;
	uint8_t byte;
	uint8_t value;
	uint8_t start_delay;
};

class Dma
{
	IBus* m_Bus = nullptr;
	Ppu* m_Ppu = nullptr;

public:
	Dma();
	virtual ~Dma() = default;

	void Start(uint8_t start);
	void Tick();

	bool IsTransferring();

private:
	DmaContext context = {};
};