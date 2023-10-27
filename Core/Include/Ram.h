#pragma once

#include <cstdint>
#include <vector>

class Ram
{
public:
	Ram();
	virtual ~Ram() = default;

	uint8_t ReadWorkRam(uint16_t address);
	void WriteWorkRam(uint16_t address, uint8_t value);

	uint8_t ReadHighRam(uint16_t address);
	void WriteHighRam(uint16_t address, uint8_t value);

private:
	std::vector<uint8_t> m_WorkRam;
	std::vector<uint8_t> m_HighRam;

	const uint16_t m_BaseWorkRamAddress = 0xC000;
	const uint16_t m_BaseHighRamAddress = 0xFF80;
};