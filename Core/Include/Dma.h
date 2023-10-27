#pragma once

#include <cstdint>

struct DmaContext
{
	bool active;
	uint8_t byte;
	uint8_t value;
	uint8_t start_delay;
};

class Dma
{
public:
	Dma() = default;
	virtual ~Dma() = default;

	void Start(uint8_t start);
	void Tick();

	bool IsTransferring();

	DmaContext context;
};