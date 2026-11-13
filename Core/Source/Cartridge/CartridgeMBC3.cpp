#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"
#include "RetroGBm/Logger.h"

#include <chrono>
#include <cstdint>

CartridgeMBC3::CartridgeMBC3(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
	m_ExternalRam.resize(cartridge_data.ram_size);
	std::fill(m_ExternalRam.begin(), m_ExternalRam.end(), 0x0);

	m_RtcRegisters.resize(5);
	std::fill(m_RtcRegisters.begin(), m_RtcRegisters.end(), 0x0);

	m_TimeSource = []
	{
		return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	};
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
		// Only the lower 7 bits select the ROM bank, and bank 0 maps to bank 1
		m_RomBank = value & 0x7F;
		if (m_RomBank == 0)
		{
			m_RomBank = 1;
		}
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
				this->LatchRTC();
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
				this->WriteRTC(index, value);
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
	// The clock follows real time, so loading a state keeps the live clock and its halt and carry flags.
	// The latch and the latched registers still come from the state, as they're what the game last read
	const RtcData live_clock = m_RtcData;
	const uint8_t live_flags = m_RtcRegisters[4] & 0xC0;

	int rtc_size = 0;
	file->read(reinterpret_cast<char*>(&rtc_size), sizeof(rtc_size));

	m_RtcRegisters.resize(5);
	file->read(reinterpret_cast<char*>(m_RtcRegisters.data()), m_RtcRegisters.size() * sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_RtcData), sizeof(m_RtcData));

	const bool latched = m_RtcData.m_RtcLatched;
	m_RtcData = live_clock;
	m_RtcData.m_RtcLatched = latched;

	m_RtcRegisters[4] = (m_RtcRegisters[4] & ~0xC0) | live_flags;
}

void CartridgeMBC3::UpdateRTC()
{
	const time_t now = m_TimeSource();
	const bool halted = (m_RtcRegisters[4] & 0x40) != 0;

	// Saves from before the day counter wrapped can hold more than 511 days. Keep the 9 bits the game has been
	// reading so its time doesn't change, without setting the carry flag as no real overflow happened
	if (m_RtcData.m_RtcDays > 0x1FF)
	{
		m_RtcData.m_RtcDays = static_cast<uint16_t>(m_RtcData.m_RtcDays & 0x1FF);
	}

	// Start counting from now if the clock has never run, is halted, or the system clock went backwards
	if (m_RtcData.m_UnixEpoch == 0 || halted || now < m_RtcData.m_UnixEpoch)
	{
		m_RtcData.m_UnixEpoch = now;
		return;
	}

	// Whole seconds only, the epoch moves by the same amount so no time is lost between updates
	int64_t total = m_RtcData.m_RtcSeconds;
	total += m_RtcData.m_RtcMinutes * 60LL;
	total += m_RtcData.m_RtcHours * 3600LL;
	total += m_RtcData.m_RtcDays * 86400LL;
	total += static_cast<int64_t>(now - m_RtcData.m_UnixEpoch);

	m_RtcData.m_RtcSeconds = static_cast<uint8_t>(total % 60);
	total /= 60;

	m_RtcData.m_RtcMinutes = static_cast<uint8_t>(total % 60);
	total /= 60;

	m_RtcData.m_RtcHours = static_cast<uint8_t>(total % 24);
	total /= 24;

	// The day counter is 9 bits, so past day 511 it wraps to 0 and sets the carry flag until the game clears it
	if (total > 0x1FF)
	{
		m_RtcRegisters[4] |= 0x80;
		total %= 0x200;
	}

	m_RtcData.m_RtcDays = static_cast<uint16_t>(total);
	m_RtcData.m_UnixEpoch = now;
}

void CartridgeMBC3::LatchRTC()
{
	this->UpdateRTC();
	this->SetRTC(m_RtcData.m_RtcSeconds, m_RtcData.m_RtcMinutes, m_RtcData.m_RtcHours, m_RtcData.m_RtcDays);
}

void CartridgeMBC3::WriteRTC(uint8_t index, uint8_t value)
{
	// Count the time up to now first, so the new value and any change to the halt flag only apply from here
	this->UpdateRTC();

	// Bits each register has: seconds, minutes, hours, day low, day high (bit 0 day bit 8, bit 6 halt, bit 7 carry)
	static constexpr uint8_t masks[5] = { 0x3F, 0x3F, 0x1F, 0xFF, 0xC1 };
	value &= masks[index];

	switch (index)
	{
		case 0:
			m_RtcData.m_RtcSeconds = value;
			break;
		case 1:
			m_RtcData.m_RtcMinutes = value;
			break;
		case 2:
			m_RtcData.m_RtcHours = value;
			break;
		case 3:
			m_RtcData.m_RtcDays = static_cast<uint16_t>((m_RtcData.m_RtcDays & 0x100) | value);
			break;
		case 4:
			m_RtcData.m_RtcDays = static_cast<uint16_t>((m_RtcData.m_RtcDays & 0xFF) | ((value & 0x1) << 8));
			break;
	}

	// The game reads back what it wrote without having to latch again
	m_RtcRegisters[index] = value;
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

void CartridgeMBC3::SetTimeSource(std::function<time_t()> time_source)
{
	m_TimeSource = time_source;
}
