#include "Ram.h"
#include <algorithm>

Ram::Ram()
{
	m_WorkRam.resize(0x2000);
	m_HighRam.resize(0x80);

	// Set both WRAM and HRAM to default of 0
	std::fill(m_WorkRam.begin(), m_WorkRam.end(), 0x0);
	std::fill(m_HighRam.begin(), m_HighRam.end(), 0x0);
}

uint8_t Ram::ReadWorkRam(uint16_t address)
{
	address -= m_BaseWorkRamAddress;
	return m_WorkRam[address];
}

void Ram::WriteWorkRam(uint16_t address, uint8_t value)
{
	address -= m_BaseWorkRamAddress;
	m_WorkRam[address] = value;
}

uint8_t Ram::ReadHighRam(uint16_t address)
{
	address -= m_BaseHighRamAddress;
	return m_HighRam[address];
}

void Ram::WriteHighRam(uint16_t address, uint8_t value)
{
	address -= m_BaseHighRamAddress;
	m_HighRam[address] = value;
}