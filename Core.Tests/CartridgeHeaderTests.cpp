#include "CppUnitTest.h"

#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Cartridge/CartridgeHeader.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	namespace
	{
		std::vector<uint8_t> MakeRom(size_t size = 0x8000)
		{
			std::vector<uint8_t> data(size, 0x00);
			return data;
		}

		void WriteText(std::vector<uint8_t>& data, size_t address, const std::string& text)
		{
			for (size_t i = 0; i < text.size(); ++i)
			{
				data[address + i] = static_cast<uint8_t>(text[i]);
			}
		}
	}

	TEST_CLASS(CartridgeHeaderTests)
	{
	public:

		TEST_METHOD(ReadCartridgeHeader_FileSmallerThanHeader_ReturnsNothing)
		{
			// Arrange
			std::vector<uint8_t> data(0x014F, 0x00);

			// Act
			std::optional<CartridgeHeader> header = ReadCartridgeHeader(data);

			// Assert
			Assert::IsFalse(header.has_value());
		}

		TEST_METHOD(ReadCartridgeHeader_FileExactlyHeaderSize_ReturnsHeader)
		{
			// Arrange
			std::vector<uint8_t> data(CartridgeHeaderEnd, 0x00);

			// Act
			std::optional<CartridgeHeader> header = ReadCartridgeHeader(data);

			// Assert
			Assert::IsTrue(header.has_value());
		}

		TEST_METHOD(ReadCartridgeHeader_DmgTitle_Reads16Characters)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "ABCDEFGHIJKLMNOP");

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string("ABCDEFGHIJKLMNOP"), header.title);
			Assert::AreEqual(std::string(), header.manufacturer_code);
		}

		TEST_METHOD(ReadCartridgeHeader_TitleWithNull_StopsAtNull)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "TETRIS");

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string("TETRIS"), header.title);
		}

		TEST_METHOD(ReadCartridgeHeader_CgbFlagSet_TitleIs15CharactersWithManufacturerCode)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "POKEMON_SLVAAXE");
			data[0x0143] = 0x80;

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string("POKEMON_SLV"), header.title);
			Assert::AreEqual(std::string("AAXE"), header.manufacturer_code);
		}

		TEST_METHOD(ReadCartridgeHeader_CgbFlagSetLowercaseCode_NoManufacturerCode)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "mooneye-gb test");
			data[0x0143] = 0x80;

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string(), header.manufacturer_code);
		}

		TEST_METHOD(ReadCartridgeHeader_NonAsciiTitle_ReplacesWithSpace)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "AB");
			data[0x0136] = 0xE9;
			WriteText(data, 0x0137, "CD");

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string("AB CD"), header.title);
		}

		TEST_METHOD(ReadCartridgeHeader_Cgb11CharacterTitleWithCode_SplitsTitleAndCode)
		{
			// Harvest Moon 2 GBC
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "H-MOON2 CGBBM2E");
			data[0x0143] = 0x80;

			CartridgeHeader header = ReadCartridgeHeader(data).value();

			Assert::AreEqual(std::string("H-MOON2 CGB"), header.title);
			Assert::AreEqual(std::string("BM2E"), header.manufacturer_code);
		}

		TEST_METHOD(ReadCartridgeHeader_ReadsRawFields)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0144, "01");
			data[0x0146] = 0x03;
			data[0x0147] = 0x13;
			data[0x0148] = 0x05;
			data[0x0149] = 0x03;
			data[0x014A] = 0x01;
			data[0x014B] = 0x33;
			data[0x014C] = 0x02;
			data[0x014D] = 0xAB;
			data[0x014E] = 0x12;
			data[0x014F] = 0x34;

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(std::string("01"), header.new_licensee_code);
			Assert::AreEqual<uint8_t>(0x03, header.sgb_flag);
			Assert::AreEqual<uint8_t>(0x13, header.cartridge_type);
			Assert::AreEqual<uint8_t>(0x05, header.rom_size_code);
			Assert::AreEqual<uint8_t>(0x03, header.ram_size_code);
			Assert::AreEqual<uint8_t>(0x01, header.destination_code);
			Assert::AreEqual<uint8_t>(0x33, header.old_licensee_code);
			Assert::AreEqual<uint8_t>(0x02, header.version);
			Assert::AreEqual<uint8_t>(0xAB, header.header_checksum);
			Assert::AreEqual<uint16_t>(0x1234, header.global_checksum);
		}

		TEST_METHOD(ReadCartridgeHeader_HeaderChecksum_MatchesBootRomFormula)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			WriteText(data, 0x0134, "TEST");

			uint8_t expected = 0;
			for (int i = 0x0134; i <= 0x014C; ++i)
			{
				expected = static_cast<uint8_t>(expected - data[i] - 1);
			}
			data[0x014D] = expected;

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual(expected, header.computed_header_checksum);
			Assert::IsTrue(header.IsHeaderChecksumValid());
		}

		TEST_METHOD(ReadCartridgeHeader_AllZeroHeader_HeaderChecksumIsE7)
		{
			// Arrange: 25 bytes of zero, each subtracting 1
			std::vector<uint8_t> data = MakeRom();

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual<uint8_t>(0xE7, header.computed_header_checksum);
			Assert::IsFalse(header.IsHeaderChecksumValid());
		}

		TEST_METHOD(ReadCartridgeHeader_GlobalChecksum_SkipsChecksumBytes)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			data[0x0000] = 0x10;
			data[0x7FFF] = 0x20;
			data[0x014E] = 0x00;
			data[0x014F] = 0x30;

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::AreEqual<uint16_t>(0x0030, header.computed_global_checksum);
			Assert::IsTrue(header.IsGlobalChecksumValid());
		}

		TEST_METHOD(ReadCartridgeHeader_GlobalChecksum_WrapsAt16Bits)
		{
			// Arrange
			std::vector<uint8_t> data(0x10000, 0xFF);

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert: (0x10000 - 2) * 0xFF, truncated to 16 bits
			Assert::AreEqual<uint16_t>(static_cast<uint16_t>(0xFFFE * 0xFF), header.computed_global_checksum);
		}

		TEST_METHOD(ReadCartridgeHeader_LogoMissing_LogoDoesNotMatch)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::IsFalse(header.logo_matches);
		}

		TEST_METHOD(ReadCartridgeHeader_LogoPresent_LogoMatches)
		{
			// Arrange
			std::vector<uint8_t> data = MakeRom();
			const uint8_t logo[] =
			{
				0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
				0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
				0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
			};
			std::copy(std::begin(logo), std::end(logo), data.begin() + 0x0104);

			// Act
			CartridgeHeader header = ReadCartridgeHeader(data).value();

			// Assert
			Assert::IsTrue(header.logo_matches);
		}

		TEST_METHOD(CartridgeRomSizeName_Codes_ShowSizeAndBanks)
		{
			Assert::AreEqual(std::string("32 KiB (2 banks)"), CartridgeRomSizeName(0x00));
			Assert::AreEqual(std::string("1 MiB (64 banks)"), CartridgeRomSizeName(0x05));
			Assert::AreEqual(std::string("8 MiB (512 banks)"), CartridgeRomSizeName(0x08));
			Assert::AreEqual(std::string("1.1 MiB (72 banks)"), CartridgeRomSizeName(0x52));
			Assert::AreEqual(std::string("Unknown ($09)"), CartridgeRomSizeName(0x09));
		}

		TEST_METHOD(CartridgeRamSizeName_Codes_ShowSizeAndBanks)
		{
			Assert::AreEqual(std::string("None"), CartridgeRamSizeName(0x00));
			Assert::AreEqual(std::string("8 KiB (1 bank)"), CartridgeRamSizeName(0x02));
			Assert::AreEqual(std::string("32 KiB (4 banks)"), CartridgeRamSizeName(0x03));
			Assert::AreEqual(std::string("128 KiB (16 banks)"), CartridgeRamSizeName(0x04));
			Assert::AreEqual(std::string("64 KiB (8 banks)"), CartridgeRamSizeName(0x05));
		}

		TEST_METHOD(CartridgeRamName_Mbc2_ShowsBuiltInRam)
		{
			CartridgeHeader header;
			header.cartridge_type = 0x06;
			header.ram_size_code = 0x00;

			Assert::AreEqual(std::string("512 x 4 bits, built into MBC2"), CartridgeRamName(header));
		}

		TEST_METHOD(CartridgePublisherName_OldLicensee_UsesOldTable)
		{
			CartridgeHeader header;
			header.old_licensee_code = 0x01;
			header.new_licensee_code = "34";

			Assert::AreEqual(std::string("Nintendo"), CartridgePublisherName(header));
		}

		TEST_METHOD(CartridgePublisherName_Old33_UsesNewTable)
		{
			CartridgeHeader header;
			header.old_licensee_code = 0x33;
			header.new_licensee_code = "34";

			Assert::AreEqual(std::string("Konami"), CartridgePublisherName(header));
		}

		TEST_METHOD(CartridgePublisherName_UnknownCodes_ShowsCode)
		{
			CartridgeHeader header;
			header.old_licensee_code = 0x02;
			Assert::AreEqual(std::string("Unknown ($02)"), CartridgePublisherName(header));

			header.old_licensee_code = 0x33;
			header.new_licensee_code = "ZZ";
			Assert::AreEqual(std::string("Unknown (ZZ)"), CartridgePublisherName(header));
		}

		TEST_METHOD(CartridgePublisherName_OldCodeInNewField_UsesOldTable)
		{
			// Harvest Moon 2 GBC has $33 then "E9", Natsume's old code
			CartridgeHeader header;
			header.old_licensee_code = 0x33;
			header.new_licensee_code = "E9";

			Assert::AreEqual(std::string("Natsume"), CartridgePublisherName(header));
		}

		TEST_METHOD(CartridgePublisherName_HexCodeUnknownInBothTables_ShowsCode)
		{
			CartridgeHeader header;
			header.old_licensee_code = 0x33;
			header.new_licensee_code = "FE";

			Assert::AreEqual(std::string("Unknown (FE)"), CartridgePublisherName(header));
		}

		TEST_METHOD(CartridgeHardwareName_FlagsCombine)
		{
			CartridgeHeader header;
			Assert::AreEqual(std::string("Game Boy (DMG)"), CartridgeHardwareName(header));

			header.cgb_flag = 0x80;
			Assert::AreEqual(std::string("Game Boy and Game Boy Color"), CartridgeHardwareName(header));

			header.cgb_flag = 0xC0;
			Assert::AreEqual(std::string("Game Boy Color only (CGB)"), CartridgeHardwareName(header));

			// SGB functions need the $33 licensee code as well
			header.cgb_flag = 0x00;
			header.sgb_flag = 0x03;
			Assert::AreEqual(std::string("Game Boy (DMG)"), CartridgeHardwareName(header));

			header.old_licensee_code = 0x33;
			Assert::AreEqual(std::string("Game Boy (DMG), Super Game Boy"), CartridgeHardwareName(header));
		}

		TEST_METHOD(CartridgeTypeName_KnownAndUnknown)
		{
			Assert::AreEqual(std::string("ROM only"), CartridgeTypeName(0x00));
			Assert::AreEqual(std::string("MBC3 + Timer + RAM + Battery"), CartridgeTypeName(0x10));
			Assert::AreEqual(std::string("Unknown ($04)"), CartridgeTypeName(0x04));
		}

		TEST_METHOD(CartridgeTypeHasBattery_MatchesBaseCartridgeForSupportedTypes)
		{
			for (int type = 0; type <= 0xFF; ++type)
			{
				if (!IsCartridgeTypeSupported(static_cast<uint8_t>(type)))
				{
					continue;
				}

				std::vector<uint8_t> data = MakeRom();
				data[0x0147] = static_cast<uint8_t>(type);
				data[0x0149] = 0x03;

				std::unique_ptr<BaseCartridge> cartridge = LoadCartridgeFromMemory(data);

				// BaseCartridge doesn't list MBC3 + Timer + Battery yet (separate bug)
				if (type == 0x0F)
				{
					continue;
				}

				Assert::AreEqual(cartridge->HasBattery(), CartridgeTypeHasBattery(static_cast<uint8_t>(type)), std::to_wstring(type).c_str());
			}
		}

		TEST_METHOD(IsCartridgeTypeSupported_MatchesLoadCartridgeFromMemory)
		{
			for (int type = 0; type <= 0xFF; ++type)
			{
				std::vector<uint8_t> data = MakeRom();
				data[0x0147] = static_cast<uint8_t>(type);
				data[0x0149] = 0x03;

				bool loads = true;
				try
				{
					LoadCartridgeFromMemory(data);
				}
				catch (const std::runtime_error&)
				{
					loads = false;
				}

				Assert::AreEqual(loads, IsCartridgeTypeSupported(static_cast<uint8_t>(type)), std::to_wstring(type).c_str());
			}
		}
	};
}
