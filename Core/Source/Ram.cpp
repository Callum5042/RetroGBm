#include "RetroGBm/Pch.h"
#include "RetroGBm/Ram.h"
#include <algorithm>
#include <iostream>

Ram::Ram()
{
	// 32KB RAM (8 banks of 4KB)
	m_WorkRam.resize(32768);
	m_HighRam.resize(0x80);

	// Set both WRAM and HRAM to default of 0
	std::fill(m_WorkRam.begin(), m_WorkRam.end(), 0x0);
	std::fill(m_HighRam.begin(), m_HighRam.end(), 0x0);
}

uint8_t Ram::ReadWorkRam(uint16_t address)
{
	if (address >= 0xC000 && address <= 0xCFFF)
	{
		return m_WorkRam[address - 0xC000];
	}
	else if (address >= 0xD000 && address <= 0xDFFF)
	{
		// Use banking
		return m_WorkRam[(address - 0xD000) + (m_Bank * 4096)];
	}
	else
	{
		std::cout << "Unsupported ReadWorkRam: 0x" << address << '\n';
		return 0xFF;
	}
}

void Ram::WriteWorkRam(uint16_t address, uint8_t value)
{
	if (address >= 0xC000 && address <= 0xCFFF)
	{
		m_WorkRam[address - 0xC000] = value;
	}
	else if (address >= 0xD000 && address <= 0xDFFF)
	{
		// Use banking
		m_WorkRam[(address - 0xD000) + (m_Bank * 4096)] = value;
	}
	else
	{
		std::cout << "Unsupported ReadWorkRam: 0x" << address << '\n';
	}
}

uint8_t Ram::ReadEchoRam(uint16_t address)
{
	uint16_t addr = (address & 0x3FFF) | (m_Bank << 14);
	return m_WorkRam[addr];
}

void Ram::WriteEchoRam(uint16_t address, uint8_t value)
{
	uint16_t addr = (address & 0x3FFF) | (m_Bank << 14);
	m_WorkRam[addr] = value;
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

void Ram::SetWorkRamBank(uint8_t value)
{
	m_Bank = value & 0b111;
	if (m_Bank == 0)
	{
		m_Bank = 1;
	}
}

void Ram::SaveState(std::fstream* file)
{
	size_t workram_size = m_WorkRam.size();
	file->write(reinterpret_cast<const char*>(&workram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_WorkRam.data()), workram_size * sizeof(uint8_t));

	size_t highram_size = m_HighRam.size();
	file->write(reinterpret_cast<const char*>(&highram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_HighRam.data()), highram_size * sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_Bank), sizeof(m_Bank));
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

	file->read(reinterpret_cast<char*>(&m_Bank), sizeof(m_Bank));
}