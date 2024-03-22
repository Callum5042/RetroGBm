#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/CartridgeMBC5.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(CartridgeMBC5Tests)
	{
	public:
		TEST_METHOD(Read_AddressIsRom_ReadsFromRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);
			data.data[0x3FFF] = 0xAB;

			uint16_t address = 0x3FFF;

			// Act
			CartridgeMBC5 cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRomBank_ReadsFromRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);
			data.data[0x3FFF] = 0xAB;

			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x2000, 0);

			// Act
			uint16_t address = 0x7FFF;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRamAndRamIsEnabled_ReadsFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0] = 0xAB;

			cartridge.SetExternalRam(std::move(ram));

			uint16_t ram_enable_address = 0x0;
			cartridge.Write(ram_enable_address, 0xA);

			// Act
			uint16_t address = 0xA000;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Read_AddressIsRamAndRamIsDisabled_DoNotReadFromRam)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0] = 0xAB;

			cartridge.SetExternalRam(std::move(ram));

			uint16_t ram_enable_address = 0x0;
			cartridge.Write(ram_enable_address, 0);

			// Act
			uint16_t address = 0xA000;
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Write_RamEnableAddressValueIs0xA_EnablesRam)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint16_t ram_enable_address = 0x0;
			cartridge.Write(ram_enable_address, 0xA);

			// Assert
			Assert::IsTrue(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RamAddressLower4BitsA_EnableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC1_RAM;
			data.data.resize(0x8000);

			CartridgeMBC5 cartridge(data);
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			cartridge.SetExternalRam(std::move(ram));

			// Act
			uint16_t address = 0x1FFF;
			cartridge.Write(address, 0x6A);

			// Assert
			Assert::IsTrue(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RamEnableAddressValueIsNot0xA_DisableRam)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			// Act
			uint16_t ram_enable_address = 0x0;
			cartridge.Write(ram_enable_address, 0);

			// Assert
			Assert::IsFalse(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Write_RamBankAddressWrites2_SetRamBank2)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			// Act
			cartridge.Write(0x4000, 2);

			// Assert
			Assert::AreEqual(0x2, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Write_RamBankAddressWritesGreaterThen0xF_DoNotSetRamBank)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x4000, 2);

			// Act
			cartridge.Write(0x4000, 0xFF);

			// Assert
			Assert::AreNotEqual(0xFF, static_cast<int>(cartridge.GetRamBank()));
			Assert::AreEqual(0x2, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Write_RomBankAddress_SetsRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);

			// Act
			cartridge.Write(0x2000, 0x65);

			// Assert
			Assert::AreEqual(0x65, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankUpperAddress_SetsRomBank9thBit)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x2000, 0);

			// Act
			cartridge.Write(0x3000, 0x1);

			// Assert
			Assert::AreEqual(0x100, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_HasRomBankLower8BitsRomBankUpperAddress_SetsRomBankExcludingLower8Bits)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x2000, 0x65);

			// Act
			cartridge.Write(0x3000, 0x1);

			// Assert
			Assert::AreEqual(0x165, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBankAddressAndRomBank9thBitSet_ExcludeWriting9thBit)
		{
			// Arrange
			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x3000, 0x1);

			// Act
			cartridge.Write(0x2000, 0x65);

			// Assert
			Assert::AreEqual(0x165, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Read_RamAddressAndRamBankIs1_ReadsFromRamBank1)
		{
			// Arrange
			std::vector<uint8_t> ram;
			ram.resize(0x8000);
			ram[0x2000] = 0xAB;

			CartridgeDataV2 data;
			CartridgeMBC5 cartridge(data);
			cartridge.SetExternalRam(std::move(ram));

			uint16_t ram_enable_address = 0x0;
			cartridge.Write(ram_enable_address, 0xA);

			uint16_t ram_bank_address = 0x4000;
			cartridge.Write(ram_bank_address, 0x1);

			// Act
			uint8_t result = cartridge.Read(0xA000);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
			Assert::IsTrue(cartridge.IsRamEnabled());
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRamBank()));
		}

		TEST_METHOD(Read_RomBankAddressAndRomBankIs1_ReadFromRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x800000); // 8MB
			data.data[0x4000] = 0xAB; // First byte of Bank 1

			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x2000, 1);

			// Act
			uint8_t result = cartridge.Read(0x4000);

			// Assert
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRomBank()));
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_RomBankUpper_OnlyReadBit0_SetsRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x800000); // 8MB
			data.data[0x4000] = 0xAB; // First byte of Bank 1

			CartridgeMBC5 cartridge(data);
			cartridge.Write(0x2000, 0);

			// Act
			cartridge.Write(0x3000, 0xFF);

			// Assert
			Assert::AreEqual(0x100, static_cast<int>(cartridge.GetRomBank()));
		}
	};
}