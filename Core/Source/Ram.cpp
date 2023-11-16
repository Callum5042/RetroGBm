#include "Pch.h"
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

void Ram::SaveState(std::fstream* file)
{
	size_t workram_size = m_WorkRam.size();
	file->write(reinterpret_cast<const char*>(&workram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_WorkRam.data()), workram_size * sizeof(uint8_t));

	size_t highram_size = m_HighRam.size();
	file->write(reinterpret_cast<const char*>(&highram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_HighRam.data()), highram_size * sizeof(uint8_t));
}

void Ram::LoadState(std::fstream* file)
{
	size_t workram_size = 0;
	file->read(reinterpret_cast<char*>(&workram_size), sizeof(size_t));

	m_WorkRam.resize(workram_size);
	file->read(reinterpret_cast<char*>(m_WorkRam.data()), m_WorkRam.size() * sizeof(uint8_t));

	size_t highram_size = 0;
	file->read(reinterpret_cast<char*>(&highram_size), sizeof(size_t));

	m_HighRam.resize(highram_size);
	file->read(reinterpret_cast<char*>(m_HighRam.data()), m_HighRam.size() * sizeof(uint8_t));
}