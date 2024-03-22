#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/CartridgeMBC1.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(CartridgeMBC1Tests)
	{
	public:
		TEST_METHOD(Read_RomHasData_ReturnsDataFromRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;
			data.data[address] = 0xAB;

			// Act
			CartridgeMBC1 cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_RomHasDataInRomBank_ReturnsDataFromRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x7FFF;
			data.data[address] = 0xAB;

			// Act
			CartridgeMBC1 cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_CartridgeHasRam_ReturnsDataFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			std::vector<uint8_t> ram;
			ram.resize(0x2000);
			ram[0x2000 - 1] = 0xAB;

			uint16_t address = 0xBFFF;

			CartridgeMBC1 cartridge(data);
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_CartridgeDoesNotHaveRam_ReturnsFF)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1;
			data.data.resize(0x8000);

			std::vector<uint8_t> ram;
			ram.resize(0x2000);
			ram[0x2000 - 1] = 0xAB;

			CartridgeMBC1 cartridge(data);
			cartridge.SetExternalRam(std::move(ram));

			uint16_t address = 0xBFFF;

			// Act
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Write_RamAddress_EnableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x1FFF;

			// Act
			CartridgeMBC1 cartridge(data);
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
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0x0);

			// Assert
			Assert::IsFalse(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RomBankSetTo0_RomBankIs1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0x0);

			// Assert
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankLower_SetsRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0x5);

			// Assert
			Assert::AreEqual(5, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankLower_SetsLower5BitsofRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0xFF);

			// Assert
			Assert::AreEqual(0x1F, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankUpper_SetsUpperRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x5FFF;

			// Act
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0xF);

			// Assert
			Assert::AreEqual(0xC1, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_BankModeValueIsBit0Is1_SetsBankModeToAdvance)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			uint16_t address = 0x7FFF;

			// Act
			CartridgeMBC1 cartridge(data);
			cartridge.Write(address, 0xF);

			// Assert
			Assert::IsTrue(cartridge.GetBankMode() == BankMode::RAM);
		}

		TEST_METHOD(Read_RomBankModeAndBankIs1_ReadFromRomBank1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(65536);
			data.data[0xBFFF] = 0xAB;

			CartridgeMBC1 cartridge(data);
			cartridge.Write(0x2000, 2); // Sets ROM bank 2
			cartridge.Write(0x6000, static_cast<int>(BankMode::ROM)); // Sets ROM bank mode

			// Act
			uint16_t address = 0x7FFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_RamBankModeAndBankIs1_DoNotReadFromRomBank1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(65536);
			data.data[0xFFFF] = 0xAB;

			CartridgeMBC1 cartridge(data);
			cartridge.Write(0x2000, 2); // Sets ROM bank 2
			cartridge.Write(0x6000, static_cast<int>(BankMode::RAM)); // Sets ROM bank mode

			// Act
			uint16_t address = 0x7FFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_RamBankModeAndBankIs1_ReadFromRamBank1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(65536);

			CartridgeMBC1 cartridge(data);
			cartridge.Write(0x4000, 1); // Sets ROM bank 1
			cartridge.Write(0x6000, static_cast<int>(BankMode::RAM)); // Sets ROM bank mode

			std::vector<uint8_t> ram;
			ram.resize(0x2000 * 4);
			ram[0x2000] = 0xAB;
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint16_t address = 0xA000;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}
	};
}