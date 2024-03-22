#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/CartridgeMBC3.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

			uint16_t address = 0x1FFF;

			// Act
			CartridgeMBC3 cartridge(data);
			cartridge.Write(address, 0xA);

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
			data.data[0x9FFF] = 0xAB;

			CartridgeMBC3 cartridge(data);
			uint16_t rombank_address = 0x2000;
			cartridge.Write(rombank_address, 2);

			// Act
			uint16_t address = 0x7FFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0x2, static_cast<int>(cartridge.GetRomBank()));
		}
	};
}