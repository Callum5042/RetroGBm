#include "CppUnitTest.h"

#include <RetroGBm/TileData.h>

#include <array>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TileDataTests
{
	// Example tile from https://gbdev.io/pandocs/Tile_Data.html
	const std::array<uint8_t, 16> PandocsTile = { 0x3C, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x5E, 0x7E, 0x0A, 0x7C, 0x56, 0x38, 0x7C };

	TEST_CLASS(TileDataTests)
	{
	public:
		TEST_METHOD(DecodeRow_FirstByteIsLowBit)
		{
			// Act
			std::array<uint8_t, 8> row = TileData::DecodeRow(0x3C, 0x7E);

			// Assert
			const std::array<uint8_t, 8> expected = { 0, 2, 3, 3, 3, 3, 2, 0 };
			Assert::IsTrue(expected == row);
		}

		TEST_METHOD(DecodeRow_OnlyFirstByte_ReturnsColour1)
		{
			// Act
			std::array<uint8_t, 8> row = TileData::DecodeRow(0x81, 0x00);

			// Assert
			const std::array<uint8_t, 8> expected = { 1, 0, 0, 0, 0, 0, 0, 1 };
			Assert::IsTrue(expected == row);
		}

		TEST_METHOD(DecodeRow_OnlySecondByte_ReturnsColour2)
		{
			// Act
			std::array<uint8_t, 8> row = TileData::DecodeRow(0x00, 0x81);

			// Assert
			const std::array<uint8_t, 8> expected = { 2, 0, 0, 0, 0, 0, 0, 2 };
			Assert::IsTrue(expected == row);
		}

		TEST_METHOD(DecodeTile_PandocsExample)
		{
			// Act
			std::array<uint8_t, 64> tile = TileData::DecodeTile(PandocsTile.data());

			// Assert
			const std::array<uint8_t, 64> expected = {
				0, 2, 3, 3, 3, 3, 2, 0,
				0, 3, 0, 0, 0, 0, 3, 0,
				0, 3, 0, 0, 0, 0, 3, 0,
				0, 3, 0, 0, 0, 0, 3, 0,
				0, 3, 1, 3, 3, 3, 3, 0,
				0, 1, 1, 1, 3, 1, 3, 0,
				0, 3, 1, 3, 1, 3, 2, 0,
				0, 2, 3, 3, 3, 2, 0, 0,
			};
			Assert::IsTrue(expected == tile);
		}

		TEST_METHOD(DecodeSheet_ReturnsSheetSize)
		{
			// Arrange
			std::vector<uint8_t> bank(TileData::BytesPerBank, 0);

			// Act
			std::vector<uint8_t> sheet = TileData::DecodeSheet(bank.data());

			// Assert
			Assert::AreEqual(static_cast<size_t>(128 * 192), sheet.size());
		}

		TEST_METHOD(DecodeSheet_PlacesTilesSixteenPerRow)
		{
			// Arrange: fill tile 17 (column 1, row 1) and tile 383 (last) with colour 3
			std::vector<uint8_t> bank(TileData::BytesPerBank, 0);
			std::fill(bank.begin() + 17 * 16, bank.begin() + 18 * 16, static_cast<uint8_t>(0xFF));
			std::fill(bank.end() - 16, bank.end(), static_cast<uint8_t>(0xFF));

			// Act
			std::vector<uint8_t> sheet = TileData::DecodeSheet(bank.data());

			// Assert
			auto at = [&sheet](int x, int y) { return sheet[y * TileData::SheetWidth + x]; };
			Assert::AreEqual(static_cast<uint8_t>(0), at(7, 7));
			Assert::AreEqual(static_cast<uint8_t>(3), at(8, 8));
			Assert::AreEqual(static_cast<uint8_t>(3), at(15, 15));
			Assert::AreEqual(static_cast<uint8_t>(0), at(16, 8));
			Assert::AreEqual(static_cast<uint8_t>(0), at(8, 16));
			Assert::AreEqual(static_cast<uint8_t>(3), at(120, 184));
			Assert::AreEqual(static_cast<uint8_t>(3), at(127, 191));
			Assert::AreEqual(static_cast<uint8_t>(0), at(119, 191));
		}
	};
}
