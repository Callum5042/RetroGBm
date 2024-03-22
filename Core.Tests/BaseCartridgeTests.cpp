#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Cartridge/CartridgeROM.h>
#include <RetroGBm/Cartridge/CartridgeMBC1.h>
#include <RetroGBm/Cartridge/CartridgeMBC3.h>
#include <RetroGBm/Cartridge/CartridgeMBC5.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	class TestCartridge : public BaseCartridge
	{
	public:
		TestCartridge(CartridgeDataV2 data) : BaseCartridge(data) {}

		// Inherited via BaseCartridge
		uint8_t Read(uint16_t address) override
		{
			return 0;
		}

		void Write(uint16_t address, uint8_t value) override
		{
		}

		void SaveState(std::fstream* file) override
		{
		}

		void LoadState(std::fstream* file) override
		{
		}
	};

	TEST_CLASS(CartridgeTests)
	{
	public:

		/// <summary>
		///  ROM
		/// </summary>

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsROMONLY_ReturnsCartridgeROM)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x0;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeROM* type = dynamic_cast<CartridgeROM*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsROM_RAM_ReturnsCartridgeROM)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x8;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeROM* type = dynamic_cast<CartridgeROM*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsROM_RAM_BATTERY_ReturnsCartridgeROM)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x9;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeROM* type = dynamic_cast<CartridgeROM*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		/// <summary>
		///  MB1
		/// </summary>

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC1_ReturnsCartridgeMBC1)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x1;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC1* type = dynamic_cast<CartridgeMBC1*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC1_RAM_ReturnsCartridgeMBC1)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x2;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC1* type = dynamic_cast<CartridgeMBC1*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC1_RAM_BATTERY_ReturnsCartridgeMBC1)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x3;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC1* type = dynamic_cast<CartridgeMBC1*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		/// <summary>
		///  MB3
		/// </summary>

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC3_ReturnsCartridgeMBC3)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x11;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC3* type = dynamic_cast<CartridgeMBC3*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC3_RAM_ReturnsCartridgeMBC3)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x12;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC3* type = dynamic_cast<CartridgeMBC3*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC3_RAM_BATTERY_ReturnsCartridgeMBC3)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x13;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC3* type = dynamic_cast<CartridgeMBC3*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		/// <summary>
		///  MB5
		/// </summary>

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC5_ReturnsCartridgeMBC5)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x19;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC5* type = dynamic_cast<CartridgeMBC5*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC5_RAM_ReturnsCartridgeMBC5)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x1A;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC5* type = dynamic_cast<CartridgeMBC5*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(LoadCartridgeFromMemory_CartrideTypeIsMBC5_RAM_BATTERY_ReturnsCartridgeMBC5)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x1B;

			// Act
			std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

			// Assert
			CartridgeMBC5* type = dynamic_cast<CartridgeMBC5*>(cartridge.get());
			Assert::IsNotNull(type);
		}

		TEST_METHOD(BuildCartridgeData_MapHeaderBytesToCartridgeData_ReturnsCartridgeData)
		{
			// Arrange
			std::vector<uint8_t> data;
			data.resize(0x2000);
			data[0x0147] = 0x1;

			// Act
			CartridgeDataV2 cartridge = BuildCartridgeData(data);

			// Assert
			Assert::AreEqual(data.size(), cartridge.data.size());
			Assert::IsTrue(CartridgeTypeV2::MBC1 == cartridge.cartridge_type);
		}

		TEST_METHOD(CartridgeTypeToString_CartridgeTypeIsMBC1_ReturnsMBC1String)
		{
			// Act
			std::string cartridge_type_name = CartridgeTypeToString(CartridgeTypeV2::MBC1);

			// Assert
			Assert::AreEqual("MBC1", cartridge_type_name.c_str());
		}

		TEST_METHOD(CartridgeTypeToString_CartridgeTypeIsUnknown_ReturnsNAString)
		{
			// Act
			std::string cartridge_type_name = CartridgeTypeToString(static_cast<CartridgeTypeV2>(0x4));

			// Assert
			Assert::AreEqual("N/A", cartridge_type_name.c_str());
		}

		TEST_METHOD(SetExternalRam_SetsExternalRamToData)
		{
			// Arrange
			CartridgeDataV2 cartridge_data;
			cartridge_data.cartridge_type = CartridgeTypeV2::ROM_RAM;

			std::vector<uint8_t> ram;
			ram.resize(0x2000);
			ram[0] = 0xAB;

			// Act
			TestCartridge cartridge(cartridge_data);
			cartridge.SetExternalRam(std::move(ram));

			// Assert
			std::vector<uint8_t> result = cartridge.GetExternalRam();
			Assert::AreEqual(0xAB, static_cast<int>(result[0]));
		}
	};
}