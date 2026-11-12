#include "CppUnitTest.h"
#include "NullDisplayOutput.h"
#include "NullNetworkOutput.h"
#include "NullSoundOutput.h"
#include "VideoTestFixture.h"

#include <RetroGBm/Cartridge/CartridgeMBC2.h>
#include <RetroGBm/Cpu.h>
#include <RetroGBm/Emulator.h>

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(CartridgeMBC2Tests)
	{
	public:
		TEST_METHOD(Constructor_HeaderHasNoRam_Allocates512BytesOfRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			// Act
			CartridgeMBC2 cartridge(data);

			// Assert
			Assert::AreEqual(512, static_cast<int>(cartridge.GetExternalRam().size()));
		}

		TEST_METHOD(Read_RomHasData_ReturnsDataFromRom)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);
			data.data[0x3FFF] = 0xAB;

			// Act
			CartridgeMBC2 cartridge(data);
			uint8_t result = cartridge.Read(0x3FFF);

			// Assert
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_RomBankAddressBit8Set_SetsRomBank)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x4000 * 16);
			data.data[0x4000 * 5] = 0xAB;

			CartridgeMBC2 cartridge(data);

			// Act
			cartridge.Write(0x2100, 0x05);
			uint8_t result = cartridge.Read(0x4000);

			// Assert
			Assert::AreEqual(5, static_cast<int>(cartridge.GetRomBank()));
			Assert::AreEqual(0xAB, static_cast<int>(result));
		}

		TEST_METHOD(Write_RomBankSetTo0_RomBankIs1)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x4000 * 16);

			CartridgeMBC2 cartridge(data);
			cartridge.Write(0x0100, 0x05);

			// Act
			cartridge.Write(0x0100, 0x00);

			// Assert
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RomBank_OnlyUsesLower4Bits)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x4000 * 16);

			CartridgeMBC2 cartridge(data);

			// Act
			cartridge.Write(0x3FFF, 0xF3);

			// Assert
			Assert::AreEqual(3, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RamEnableAddressBit8Clear_EnablesRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);

			// Act
			cartridge.Write(0x3EFF, 0x0A);

			// Assert
			Assert::IsTrue(cartridge.IsRamEnabled());
			Assert::AreEqual(1, static_cast<int>(cartridge.GetRomBank()));
		}

		TEST_METHOD(Write_RamEnableValueNot0A_DisablesRam)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);
			cartridge.Write(0x0000, 0x0A);

			// Act
			cartridge.Write(0x0000, 0x00);

			// Assert
			Assert::IsFalse(cartridge.IsRamEnabled());
		}

		TEST_METHOD(Read_RamDisabled_ReturnsFF)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);
			cartridge.Write(0x0000, 0x0A);
			cartridge.Write(0xA000, 0x05);
			cartridge.Write(0x0000, 0x00);

			// Act
			uint8_t result = cartridge.Read(0xA000);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Read_RamEnabled_UpperNibbleReadsAs1s)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);
			cartridge.Write(0x0000, 0x0A);

			// Act
			cartridge.Write(0xA000, 0x35);
			uint8_t result = cartridge.Read(0xA000);

			// Assert
			Assert::AreEqual(0xF5, static_cast<int>(result));
			Assert::AreEqual(0x05, static_cast<int>(cartridge.GetExternalRam()[0]));
		}

		TEST_METHOD(Read_RamEnabled_RamEchoesEvery512Bytes)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);
			cartridge.Write(0x0000, 0x0A);

			// Act
			cartridge.Write(0xA1FF, 0x07);

			// Assert
			Assert::AreEqual(0xF7, static_cast<int>(cartridge.Read(0xA3FF)));
			Assert::AreEqual(0xF7, static_cast<int>(cartridge.Read(0xBFFF)));
		}

		TEST_METHOD(Write_RamDisabledWithBattery_CallsWriteRamCallback)
		{
			// Arrange
			CartridgeDataV2 data;
			data.cartridge_type = CartridgeTypeV2::MBC2_BATTERY;
			data.data.resize(0x8000);

			CartridgeMBC2 cartridge(data);

			bool called = false;
			cartridge.SetWriteRamCallback([&] { called = true; });
			cartridge.Write(0x0000, 0x0A);

			// Act
			cartridge.Write(0x0000, 0x00);

			// Assert
			Assert::IsTrue(called);
		}

		TEST_METHOD(MooneyeMbc2EmulatorOnly)
		{
			std::ostringstream failures;
			int tested = 0;

			for (const auto& entry : std::filesystem::directory_iterator(VideoTests::RomRoot() / "Gekkio" / "emulator-only" / "mbc2"))
			{
				if (entry.path().extension() != ".gb")
				{
					continue;
				}

				++tested;

				// Keep battery saves out of the working directory
				std::filesystem::path battery_path = std::filesystem::temp_directory_path() / ("RetroGBm-" + entry.path().stem().string() + ".save");
				std::filesystem::remove(battery_path);

				NullDisplayOutput display;
				NullSoundOutput sound;
				NullNetworkOutput network;
				Emulator emulator(&display, &sound, &network);
				emulator.SetFramePacingEnabled(false);
				emulator.SetBatteryPath(battery_path.string());

				bool passed = false;
				if (emulator.LoadRom(entry.path().string()))
				{
					for (int i = 0; i < 5'000'000; ++i)
					{
						emulator.Tick();

						Cpu* cpu = emulator.GetCpu();
						if (cpu->GetRegister(RegisterType16::REG_BC) == 0x0305 && cpu->GetRegister(RegisterType16::REG_DE) == 0x080D && cpu->GetRegister(RegisterType16::REG_HL) == 0x1522)
						{
							passed = true;
							break;
						}

						if (emulator.GetOpCode() == 0x40 && cpu->GetRegister(RegisterType16::REG_BC) == 0x4242 && cpu->GetRegister(RegisterType16::REG_DE) == 0x4242)
						{
							break;
						}
					}
				}

				std::filesystem::remove(battery_path);

				std::string result = entry.path().filename().string() + (passed ? ": PASS\n" : ": FAIL\n");
				Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(result.c_str());
				if (!passed)
				{
					failures << result;
				}
			}

			const std::string message = failures.str();
			Assert::AreEqual(7, tested, L"Bundled MBC2 ROMs are missing");
			Assert::IsTrue(message.empty(), std::wstring(message.begin(), message.end()).c_str());
		}
	};
}
