#pragma once

#include "BaseCartridge.h"
#include <cstdint>
#include "../HighTimer.h"

struct RtcData
{
	uint8_t m_RtcSeconds = 0;
	uint8_t m_RtcMinutes = 0;
	uint8_t m_RtcHours = 0;
	uint16_t m_RtcDays = 0;

	bool m_RtcLatched = false;
};

class CartridgeMBC3 : public BaseCartridge
{
public:
	CartridgeMBC3(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeMBC3() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;

	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	void TickRTC();
	void SetRTC(uint8_t seconds, uint8_t minutes, uint8_t hours, uint16_t days);
	bool IsRtcEnabled() const;

	inline bool IsRamEnabled() const
	{
		return m_ExternalRamEnabled && !m_ExternalRam.empty();
	}

	inline uint8_t GetRomBank() const
	{
		return m_RomBank;
	}

	inline uint8_t GetRamBank() const
	{
		return m_RamBank;
	}

protected:
	bool m_ExternalRamEnabled = false;
	bool m_RtcEnabled = false;
	uint8_t m_RomBank = 1;
	uint8_t m_RamBank = 0;


	// Real-Time Clock (RTC)
	HighTimer m_RealTimeClockTimer;

public:
	std::vector<uint8_t> m_RtcRegisters;
	RtcData m_RtcData;
};