#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/CartridgeMBC3.h>

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
	// MBC3 cartridge with a clock, driven by the time in 'now' instead of the system clock
	std::unique_ptr<CartridgeMBC3> CreateRtcCartridge(const time_t* now)
	{
		CartridgeDataV2 data;
		data.cartridge_type = CartridgeTypeV2::MBC3_TIMER_RAM_BATTERY;
		data.data.resize(0x8000);
		data.ram_size = 0x2000;

		auto cartridge = std::make_unique<CartridgeMBC3>(data);
		cartridge->SetWriteRamCallback([] {});
		cartridge->SetTimeSource([now] { return *now; });
		return cartridge;
	}

	// Same sequence games use: enable RAM, then write 0 and 1 to the latch register
	void LatchClock(CartridgeMBC3& cartridge)
	{
		cartridge.Write(0x0000, 0x0A);
		cartridge.Write(0x6000, 0x00);
		cartridge.Write(0x6000, 0x01);
	}

	// Register is 0x08 (seconds) to 0x0C (day high)
	uint8_t ReadClockRegister(CartridgeMBC3& cartridge, uint8_t clock_register)
	{
		cartridge.Write(0x4000, clock_register);
		return cartridge.Read(0xA000);
	}

	void WriteClockRegister(CartridgeMBC3& cartridge, uint8_t clock_register, uint8_t value)
	{
		cartridge.Write(0x4000, clock_register);
		cartridge.Write(0xA000, value);
	}
}

namespace CoreTests
{
	TEST_CLASS(CartridgeMBC3Tests)
	{
	public:
		TEST_METHOD(Read_AddressIsRom_ReadsFromNonBankedRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);
			data.data[0x3FFF] = 0xAB;

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRomBank_ReadsFromBankedRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);
			data.data[0x7FFF] = 0xAB;

			uint16_t address = 0x7FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_RamAddress_EnableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint16_t address = 0x1FFF;
			cartridge.Write(address, 0xA);

			// Assert
			Assert::IsTrue(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RamAddressLower4BitsA_EnableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint16_t address = 0x1FFF;
			cartridge.Write(address, 0x6A);

			// Assert
			Assert::IsTrue(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RamAddress_DisableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x1FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0x0);

			// Assert
			Assert::IsFalse(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RomBankAddress_SetsRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0x5);

			// Assert
			Assert::AreEqual(5, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankAddressValueIs0_SetsRomBankTo1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0x0);

			// Assert
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RamBankAddress_SetRamBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x5FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0x3);

			// Assert
			Assert::AreEqual(3, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Write_RamBankAddressIsGreaterThen3_DoNotSetRamBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x5FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0x7);

			// Assert
			Assert::AreEqual(0, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Read_AddressIsRamAndRamIsEnabled_ReadsFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x2000 - 1] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			// Enable RAM
			cartridge.Write(0x0, 0xA);

			// Act
			uint16_t address = 0xBFFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRamAndRamIsDisabled_DoNotReadFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x2000 - 1] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			// Disable RAM
			cartridge.Write(0x0, 0x0);

			// Act
			uint16_t address = 0xBFFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Write_AddressIsRamAndRamIsEnabled_WritesToRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x2000 - 1] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			// Enable RAM
			cartridge.Write(0x0, 0xA);

			// Act
			uint16_t address = 0xBFFF;
			cartridge.Write(address, 0x23);

			// Assert
			cartridge.Write(0x0, 0xA);
			uint8_t result = cartridge.Read(address);
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0x23, static_cast<int>(result));
		}

		TEST_METHOD(Write_AddressIsRamAndRamIsDisabled_DoNotReadFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x2000 - 1] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			// Disable RAM
			cartridge.Write(0x0, 0x0);

			// Act
			uint16_t address = 0xBFFF;
			cartridge.Write(address, 0x23);

			// Assert
			cartridge.Write(0x0, 0xA);
			uint8_t result = cartridge.Read(address);
			Assert::AreNotSame(0x23, static_cast<int>(result));
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRamAndBankNumberIs1_ReadsFromBankedRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x4000 - 1] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			uint16_t enable_ram_address = 0x0;
			cartridge.Write(enable_ram_address, 0xA);

			uint16_t set_rambank_address = 0x4000;
			cartridge.Write(set_rambank_address, 0x1);

			// Act
			uint16_t address = 0xBFFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0x1, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Write_AddressIsRamAndBankNumberIs1_ReadsFromBankedRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC3 cartridge(data);
			
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			cartridge.SetExternalRam(std::move(ram));

			uint16_t enable_ram_address = 0x0;
			cartridge.Write(enable_ram_address, 0xA);

			uint16_t set_rambank_address = 0x4000;
			cartridge.Write(set_rambank_address, 0x1);

			// Act
			uint16_t address = 0xBFFF;
			cartridge.Write(address, 0xAB);

			// Assert
			uint8_t result = cartridge.Read(address);
			Assert::AreEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0x1, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Read_AddressIsRomBankAndRomBankIs2_ReadsFromRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x10000);
			data.data[0x8000] = 0xAB;

			CartridgeMBC3 cartridge(data);
			uint16_t rombank_address = 0x2000;
			cartridge.Write(rombank_address, 2);

			// Act
			uint16_t address = 0x4000;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0x2, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Latch_TimePassedWhileRamDisabled_ClockCountsAllOfIt)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Game disables RAM and leaves the clock latched, as it does between reads
			cartridge->Write(0x0000, 0x00);
			now += 5;

			// Act
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(5, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
		}

		TEST_METHOD(Latch_TimePassedOverDays_SetsAllClockRegisters)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// 301 days, 2 hours, 3 minutes and 4 seconds
			now += (301 * 86400) + (2 * 3600) + (3 * 60) + 4;

			// Act
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(4, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(3, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
			Assert::AreEqual(2, static_cast<int>(ReadClockRegister(*cartridge, 0x0A)));
			Assert::AreEqual(301 & 0xFF, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x0C) & 0x1));
		}

		TEST_METHOD(Latch_ManyReadsOverAMinute_ClockDoesNotDrift)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Act
			for (int i = 0; i < 60; ++i)
			{
				now += 1;
				LatchClock(*cartridge);
				cartridge->Write(0x0000, 0x00);
			}

			// Assert
			LatchClock(*cartridge);
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
		}

		TEST_METHOD(Latch_ClockHalted_ClockOnlyCountsWhileRunning)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Run for 2 seconds, halt for 10, then run for 3
			now += 2;
			WriteClockRegister(*cartridge, 0x0C, 0x40);
			now += 10;
			WriteClockRegister(*cartridge, 0x0C, 0x00);
			now += 3;

			// Act
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(5, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
		}

		TEST_METHOD(Latch_SystemClockWentBackwards_ClockHoldsThenCarriesOn)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			now += 10;
			LatchClock(*cartridge);

			// Act
			now -= 100;
			LatchClock(*cartridge);
			uint8_t seconds_after_going_back = ReadClockRegister(*cartridge, 0x08);

			now += 4;
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(10, static_cast<int>(seconds_after_going_back));
			Assert::AreEqual(14, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
		}

		TEST_METHOD(LoadState_StateSavedEarlier_KeepsLiveClock)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			auto path = std::filesystem::temp_directory_path() / "RetroGBm-mbc3-rtc.state";
			std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

			now += 10;
			cartridge->SaveState(&file);

			// Halt the live clock for 50 of the next 90 seconds, so it no longer matches the state's clock plus the time passed
			WriteClockRegister(*cartridge, 0x0C, 0x40);
			now += 50;
			WriteClockRegister(*cartridge, 0x0C, 0x00);
			now += 40;

			// Act
			file.seekg(0);
			cartridge->LoadState(&file);
			file.close();
			std::filesystem::remove(path);

			// Assert
			LatchClock(*cartridge);
			Assert::AreEqual(50, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
		}

		TEST_METHOD(WriteClock_SecondsWritten_ClockCountsOnFromWrittenValue)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Act
			WriteClockRegister(*cartridge, 0x08, 30);
			now += 5;
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(35, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
		}

		TEST_METHOD(WriteClock_AllRegistersWritten_ClockCountsOnFromWrittenTime)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Day 300, 23:59:58
			WriteClockRegister(*cartridge, 0x08, 58);
			WriteClockRegister(*cartridge, 0x09, 59);
			WriteClockRegister(*cartridge, 0x0A, 23);
			WriteClockRegister(*cartridge, 0x0B, 300 & 0xFF);
			WriteClockRegister(*cartridge, 0x0C, 0x01);

			// Act
			now += 3;
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x0A)));
			Assert::AreEqual(301 & 0xFF, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x0C) & 0x1));
		}

		TEST_METHOD(WriteClock_HaltedWhileSettingTime_ClockStartsFromWrittenTimeWhenResumed)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Halt, set the time to 12:00:00 on day 0, then resume
			WriteClockRegister(*cartridge, 0x0C, 0x40);
			now += 100;
			WriteClockRegister(*cartridge, 0x08, 0);
			WriteClockRegister(*cartridge, 0x09, 0);
			WriteClockRegister(*cartridge, 0x0A, 12);
			WriteClockRegister(*cartridge, 0x0B, 0);
			now += 100;
			WriteClockRegister(*cartridge, 0x0C, 0x00);

			// Act
			now += 5;
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(5, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
			Assert::AreEqual(12, static_cast<int>(ReadClockRegister(*cartridge, 0x0A)));
			Assert::AreEqual(0, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
		}

		TEST_METHOD(WriteClock_ValueHasUnusedBits_UnusedBitsAreDropped)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Act
			WriteClockRegister(*cartridge, 0x08, 0xFF);
			WriteClockRegister(*cartridge, 0x09, 0xFF);
			WriteClockRegister(*cartridge, 0x0A, 0xFF);
			WriteClockRegister(*cartridge, 0x0C, 0xFF);
			
			// Assert
			Assert::AreEqual(0x3F, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(0x3F, static_cast<int>(ReadClockRegister(*cartridge, 0x09)));
			Assert::AreEqual(0x1F, static_cast<int>(ReadClockRegister(*cartridge, 0x0A)));
			Assert::AreEqual(0xC1, static_cast<int>(ReadClockRegister(*cartridge, 0x0C)));
		}

		TEST_METHOD(Latch_DayCounterPassesDay511_WrapsAndSetsCarry)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Day 510
			WriteClockRegister(*cartridge, 0x0B, 510 & 0xFF);
			WriteClockRegister(*cartridge, 0x0C, 0x01);

			// Act
			now += 3 * 86400;
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
			Assert::AreEqual(0x80, static_cast<int>(ReadClockRegister(*cartridge, 0x0C)));
		}

		TEST_METHOD(Latch_CarrySet_StaysSetUntilGameClearsIt)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);
			LatchClock(*cartridge);

			// Day 511, then wrap to day 0
			WriteClockRegister(*cartridge, 0x0B, 511 & 0xFF);
			WriteClockRegister(*cartridge, 0x0C, 0x01);
			now += 86400;
			LatchClock(*cartridge);

			// Act
			now += 86400;
			LatchClock(*cartridge);
			uint8_t day_high_before_clearing = ReadClockRegister(*cartridge, 0x0C);

			WriteClockRegister(*cartridge, 0x0C, 0x00);
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(0x80, static_cast<int>(day_high_before_clearing));
			Assert::AreEqual(0x00, static_cast<int>(ReadClockRegister(*cartridge, 0x0C)));
			Assert::AreEqual(1, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
		}

		TEST_METHOD(Latch_SaveFromBeforeDaysWrapped_KeepsDaysGameSawWithoutCarry)
		{
			// Arrange
			time_t now = 1000;
			auto cartridge = CreateRtcCartridge(&now);

			// Battery saves from before the fix could count days since 1970, e.g. 20737 (0x5101)
			cartridge->m_RtcData.m_RtcDays = 20737;
			cartridge->m_RtcData.m_UnixEpoch = now - 10;

			// Act
			LatchClock(*cartridge);

			// Assert
			Assert::AreEqual(10, static_cast<int>(ReadClockRegister(*cartridge, 0x08)));
			Assert::AreEqual(0x01, static_cast<int>(ReadClockRegister(*cartridge, 0x0B)));
			Assert::AreEqual(0x01, static_cast<int>(ReadClockRegister(*cartridge, 0x0C)));
		}
	};
}