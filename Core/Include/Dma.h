#pragma once

#include <cstdint>
#include <fstream>

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
	inline uint8_t GetLengthModeStart() { return m_LengthModeStart; }

	// Gameboy colour
	void StartCGB(uint8_t value);
	void SetSource(uint16_t address, uint8_t value);
	void SetDestination(uint16_t address, uint8_t value);

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

private:
	DmaContext context = {};

	// Gameboy colour
	bool m_ColourDMA = false;
	uint16_t m_Source = 0;
	uint16_t m_Destination = 0;

	bool m_GeneralPurposeDMA = false;
	uint8_t m_TransferLength = 0;

	uint8_t m_LengthModeStart = 0;
};