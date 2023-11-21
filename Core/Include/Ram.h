#pragma once

#include <cstdint>
#include <vector>
#include <fstream>

class Ram
{
public:
	Ram();
	virtual ~Ram() = default;

	uint8_t ReadWorkRam(uint16_t address);
	void WriteWorkRam(uint16_t address, uint8_t value);

	uint8_t ReadHighRam(uint16_t address);
	void WriteHighRam(uint16_t address, uint8_t value);

	void SetWorkRamBank(uint8_t value);
	inline uint8_t GetWorkRamBank() const { return m_Bank; }

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	// Only used to verify tests
	inline const std::vector<uint8_t>& GetWorkRam() { return m_WorkRam; }

private:
	std::vector<uint8_t> m_WorkRam;
	std::vector<uint8_t> m_HighRam;

	const uint16_t m_BaseWorkRamAddress = 0xC000;
	const uint16_t m_BaseWorkRamBankAddress = 0xD000;
	const uint16_t m_BaseHighRamAddress = 0xFF80;

	uint8_t m_Bank = 1;
};