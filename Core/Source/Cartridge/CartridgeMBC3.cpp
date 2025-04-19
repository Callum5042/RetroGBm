#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"
#include "RetroGBm/Logger.h"

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;
using namespace std::chrono;

CartridgeMBC3::CartridgeMBC3(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
	m_ExternalRam.resize(cartridge_data.ram_size);
	std::fill(m_ExternalRam.begin(), m_ExternalRam.end(), 0x0);

	m_RtcRegisters.resize(5);
	std::fill(m_RtcRegisters.begin(), m_RtcRegisters.end(), 0x0);

	m_RealTimeClockTimer.Start();
}

uint8_t CartridgeMBC3::Read(uint16_t address)
{
	if (address >= 0 && address <= 0x3FFF)
	{
		return m_CartridgeData.data[address];
	}
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		uint16_t bank_number = m_RomBank;
		int offset = ((address - 0x4000) + (0x4000 * bank_number)) % m_CartridgeData.data.size();
		return m_CartridgeData.data[offset];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (this->IsRamEnabled())
		{
			if (m_RamBank >= 0x0 && m_RamBank <= 0x03)
			{
				// RAM
				int offset = ((address - 0xA000) + (m_RamBank * 0x2000)) % m_ExternalRam.size();
				return m_ExternalRam[offset];
			}
			else if (m_RamBank >= 0x08 && m_RamBank <= 0x0C)
			{
				// Real-Time Clock (RTC)
				uint8_t index = (m_RamBank - 0x08);
				return m_RtcRegisters[index];
			}
		}
	}

	return 0xFF;
}

void CartridgeMBC3::Write(uint16_t address, uint8_t value)
{
	if (address >= 0x0 && address <= 0x1FFF)
	{
		m_ExternalRamEnabled = (value & 0xF) == 0xA;
		m_RtcEnabled = (value & 0xF) == 0xA;

		// Save to file each time we disable the ram
		if (this->HasBattery() && !m_ExternalRamEnabled)
		{
			m_WriteRamCallback();
		}
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		// Bank must be at least 1
		if (value == 0)
		{
			value = 1;
		}

		// Sets RAM bank to lower 7 bits
		m_RomBank = value & 0x7F;
	}

	else if (address >= 0x4000 && address <= 0x5FFF)
	{
		if (value <= 3)
		{
			// Only set RAM bank if value is less then 3
			m_RamBank = value & 0x3;
		}

		// Real-Time Clock (RTC)
		if (value >= 0x08 && value <= 0x0C)
		{
			m_RamBank = value;
		}
	}
	else if (address >= 0x6000 && address <= 0x7FFF)
	{
		if (m_RtcEnabled)
		{
			if (value == 0x0 && m_RtcData.m_RtcLatched)
			{
				m_RtcData.m_RtcLatched = false;
			}
			else if (value == 0x1 && !m_RtcData.m_RtcLatched)
			{
				m_RtcData.m_RtcLatched = true;
				this->SetRTC(m_RtcData.m_RtcSeconds, m_RtcData.m_RtcMinutes, m_RtcData.m_RtcHours, m_RtcData.m_RtcDays);
			}
		}
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		// Writes to RAM
		if (this->IsRamEnabled())
		{
			if (m_RamBank >= 0x0 && m_RamBank <= 0x03)
			{
				// RAM
				int offset = ((address - 0xA000) + (m_RamBank * 0x2000)) % m_ExternalRam.size();
				m_ExternalRam[offset] = value;
			}
		}

		// Real-Time Clock (RTC)
		if (m_RtcEnabled)
		{
			if (m_RamBank >= 0x08 && m_RamBank <= 0x0C)
			{
				uint8_t index = (m_RamBank - 0x08);
				m_RtcRegisters[index] = value;
			}
		}
	}
}

void CartridgeMBC3::SaveState(std::fstream* file)
{
	int ram_size = static_cast<int>(m_ExternalRam.size());
	file->write(reinterpret_cast<const char*>(&ram_size), sizeof(ram_size));
	file->write(reinterpret_cast<const char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
	file->write(reinterpret_cast<const char*>(&m_RomBank), sizeof(m_RomBank));
	file->write(reinterpret_cast<const char*>(&m_RamBank), sizeof(m_RamBank));

	// RTC
	int rtc_size = static_cast<int>(m_RtcRegisters.size());
	file->write(reinterpret_cast<const char*>(&rtc_size), sizeof(rtc_size));
	file->write(reinterpret_cast<const char*>(m_RtcRegisters.data()), m_RtcRegisters.size() * sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_RtcData), sizeof(m_RtcData));

	Logger::Info("Unix epoch: " + std::to_string(m_RtcData.m_UnixEpoch));
}

void CartridgeMBC3::LoadState(std::fstream* file)
{
	int ram_size = 0;
	file->read(reinterpret_cast<char*>(&ram_size), sizeof(ram_size));

	m_ExternalRam.resize(ram_size);
	file->read(reinterpret_cast<char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
	file->read(reinterpret_cast<char*>(&m_RomBank), sizeof(m_RomBank));
	file->read(reinterpret_cast<char*>(&m_RamBank), sizeof(m_RamBank));

	// RTC
	int rtc_size = 0;
	file->read(reinterpret_cast<char*>(&rtc_size), sizeof(rtc_size));

	m_RtcRegisters.resize(5);
	file->read(reinterpret_cast<char*>(m_RtcRegisters.data()), m_RtcRegisters.size() * sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_RtcData), sizeof(m_RtcData));

	auto currentTime = std::chrono::system_clock::now();
	auto lastTime = std::chrono::system_clock::from_time_t(m_RtcData.m_UnixEpoch);

	Logger::Info("MBC3 - Last unix epoch: " + std::to_string(std::chrono::system_clock::to_time_t(lastTime)));
	Logger::Info("MBC3 - Current unix epoch: " + std::to_string(std::chrono::system_clock::to_time_t(currentTime)));

	auto duration = duration_cast<seconds>(currentTime - lastTime);

	auto d = duration_cast<days>(duration);
	duration -= d;

	auto h = duration_cast<hours>(duration);
	duration -= h;

	auto m = duration_cast<minutes>(duration);
	duration -= m;

	auto s = duration_cast<seconds>(duration);

	int seconds = m_RtcData.m_RtcSeconds + (int)s.count();
	int minutes = m_RtcData.m_RtcMinutes + (int)m.count();
	int hours = m_RtcData.m_RtcHours + (int)h.count();
	int days = m_RtcData.m_RtcDays + (int)d.count();

	minutes += (seconds - (seconds % 60)) / 60;
	seconds = (seconds % 60);

	hours += (minutes - (minutes % 60)) / 60;
	minutes = minutes % 60;

	days += (hours - (hours % 24)) / 24;
	hours = hours % 24;

	m_RtcData.m_RtcSeconds = seconds;
	m_RtcData.m_RtcMinutes = minutes;
	m_RtcData.m_RtcHours = hours;
	m_RtcData.m_RtcDays = days;

	SetRTC(seconds, minutes, hours, days);
}

void CartridgeMBC3::SetRTC(uint8_t seconds, uint8_t minutes, uint8_t hours, uint16_t days)
{
	m_RtcRegisters[0] = seconds;
	m_RtcRegisters[1] = minutes;
	m_RtcRegisters[2] = hours;
	m_RtcRegisters[3] = days & 0xFF;

	m_RtcRegisters[4] &= ~0x1;
	m_RtcRegisters[4] |= ((days >> 8) & 0x1);
}

void CartridgeMBC3::TickRTC()
{
	m_RealTimeClockTimer.Tick();
	if (m_RealTimeClockTimer.TotalTime() > 1.0f)
	{
		auto currentTime = std::chrono::system_clock::now();
		m_RtcData.m_UnixEpoch = std::chrono::system_clock::to_time_t(currentTime);

		// Seconds
		m_RtcData.m_RtcSeconds++;

		if (m_RtcData.m_RtcSeconds >= 60)
		{
			// Minutes
			m_RtcData.m_RtcSeconds = 0;
			m_RtcData.m_RtcMinutes++;

			if (m_RtcData.m_RtcMinutes >= 60)
			{
				// Hours
				m_RtcData.m_RtcMinutes = 0;
				m_RtcData.m_RtcHours++;

				if (m_RtcData.m_RtcHours >= 24)
				{
					// Days
					m_RtcData.m_RtcHours = 0;
					m_RtcData.m_RtcDays++;
				}
			}
		}

		m_RealTimeClockTimer.Reset();
	}
}

bool CartridgeMBC3::IsRtcEnabled() const
{
	return (m_RtcEnabled && ((m_RtcRegisters[4] & 0x40) == 0) && !m_RtcData.m_RtcLatched);
}