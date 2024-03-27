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

	void RunHDMA();
	void RunGDMA(bool previous_active);

	void Reset();

	bool IsTransferring() const;
	inline uint8_t GetLengthModeStart() const { return m_LengthCode; }

	uint8_t GetHDMA5() const;

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
	uint16_t m_Source = 0;
	uint16_t m_Destination = 0;

	// HDMA
	bool m_Active = false;
	short m_Length;
	uint8_t m_LengthCode;
	bool m_HBlankMode;

	uint16_t m_DmaSrc;
	uint16_t m_DmaDest;
};