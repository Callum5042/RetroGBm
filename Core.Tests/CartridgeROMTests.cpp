#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/CartridgeROM.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(CartridgeROMTests)
	{
	public:
		TEST_METHOD(Read_RomHasData_ReturnsDataFromRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);

			uint16_t address = 0x7FFF;
			data.data[address] = 0xAB;

			// Act
			CartridgeROM cartridge(data);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_RomHasData_WritesAreIgnoredAndDoNotModifyData)
		{
			// Arrange
			CartridgeDataV2 data;
			data.data.resize(0x8000);

			uint16_t address = 0x7FFF;
			data.data[address] = 0xAB;

			// Act
			CartridgeROM cartridge(data);
			cartridge.Write(address, 0xFF);

			// Assert
			uint8_t result = cartridge.GetCartridgeData().data[address];
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(ReadWrite_CartridgeHasRam_WritesToRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.ram_size = 0x8000; // 8KB
			data.cartridge_type = CartridgeTypeV2::ROM_RAM;

			uint16_t address = 0xA000;

			// Act
			CartridgeROM cartridge(data);
			cartridge.Write(address, 0xAB);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_CartridgeDoesNotHaveRam_IgnoreWrites)
		{
			// Arrange
			CartridgeDataV2 data;
			data.ram_size = 0x8000; // 8KB
			data.cartridge_type = CartridgeTypeV2::ROM_ONLY;

			uint16_t address = 0xA000;

			// Act
			CartridgeROM cartridge(data);
			cartridge.Write(address, 0xAB);
			uint8_t result = cartridge.Read(address);

			// Assert
			Assert::AreNotEqual(0xAB, static_cast<int>(result));
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}
	};
}