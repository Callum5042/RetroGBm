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

		// A sheet where every pixel is its own position, so it can be found after composing
		static std::vector<uint32_t> PositionSheet(uint32_t tag)
		{
			std::vector<uint32_t> sheet(TileData::SheetWidth * TileData::SheetHeight);
			for (int y = 0; y < TileData::SheetHeight; ++y)
			{
				for (int x = 0; x < TileData::SheetWidth; ++x)
				{
					sheet[y * TileData::SheetWidth + x] = (tag << 24) | (y << 8) | x;
				}
			}

			return sheet;
		}

		static uint32_t At(const TileData::Image& image, int x, int y)
		{
			return image.pixels[static_cast<size_t>(y) * image.width + x];
		}

		TEST_METHOD(ComposeSheets_NoSheets_ReturnsEmptyImage)
		{
			// Act
			TileData::Image image = TileData::ComposeSheets({}, TileData::SheetLayout::SideBySide, 1, false, 0);

			// Assert
			Assert::AreEqual(0, image.width);
			Assert::AreEqual(0, image.height);
			Assert::IsTrue(image.pixels.empty());
		}

		TEST_METHOD(ComposeSheets_OneSheet_CopiesPixels)
		{
			// Arrange
			const std::vector<uint32_t> sheet = PositionSheet(1);

			// Act
			TileData::Image image = TileData::ComposeSheets({ sheet }, TileData::SheetLayout::SideBySide, 1, false, 0);

			// Assert
			Assert::AreEqual(128, image.width);
			Assert::AreEqual(192, image.height);
			Assert::IsTrue(sheet == image.pixels);
		}

		TEST_METHOD(ComposeSheets_SideBySide_PlacesSecondSheetToTheRight)
		{
			// Act
			TileData::Image image = TileData::ComposeSheets({ PositionSheet(1), PositionSheet(2) }, TileData::SheetLayout::SideBySide, 1, false, 0);

			// Assert
			Assert::AreEqual(256, image.width);
			Assert::AreEqual(192, image.height);
			Assert::AreEqual((1u << 24) | (5u << 8) | 127u, At(image, 127, 5));
			Assert::AreEqual((2u << 24) | (5u << 8) | 0u, At(image, 128, 5));
		}

		TEST_METHOD(ComposeSheets_Stacked_PlacesSecondSheetBelow)
		{
			// Act
			TileData::Image image = TileData::ComposeSheets({ PositionSheet(1), PositionSheet(2) }, TileData::SheetLayout::Stacked, 1, false, 0);

			// Assert
			Assert::AreEqual(128, image.width);
			Assert::AreEqual(384, image.height);
			Assert::AreEqual((1u << 24) | (191u << 8) | 3u, At(image, 3, 191));
			Assert::AreEqual((2u << 24) | (0u << 8) | 3u, At(image, 3, 192));
		}

		TEST_METHOD(ComposeSheets_Scale_RepeatsEachPixel)
		{
			// Act
			TileData::Image image = TileData::ComposeSheets({ PositionSheet(1) }, TileData::SheetLayout::SideBySide, 2, false, 0);

			// Assert
			Assert::AreEqual(256, image.width);
			Assert::AreEqual(384, image.height);

			const uint32_t pixel = (1u << 24) | (0u << 8) | 1u;
			Assert::AreEqual(pixel, At(image, 2, 0));
			Assert::AreEqual(pixel, At(image, 3, 0));
			Assert::AreEqual(pixel, At(image, 2, 1));
			Assert::AreEqual(pixel, At(image, 3, 1));
			Assert::AreNotEqual(pixel, At(image, 4, 0));
		}

		TEST_METHOD(ComposeSheets_Grid_SeparatesTiles)
		{
			// Act
			const uint32_t grid = 0xFF000000;
			TileData::Image image = TileData::ComposeSheets({ PositionSheet(1) }, TileData::SheetLayout::SideBySide, 1, true, grid);

			// Assert: 16 tiles of 8 pixels plus 17 lines across, 24 tiles plus 25 lines down
			Assert::AreEqual(16 * 9 + 1, image.width);
			Assert::AreEqual(24 * 9 + 1, image.height);

			Assert::AreEqual(grid, At(image, 0, 0));
			Assert::AreEqual(grid, At(image, 9, 5));
			Assert::AreEqual(grid, At(image, 5, 9));
			Assert::AreEqual(grid, At(image, image.width - 1, image.height - 1));

			// Pixel (0,0) is inside the border, and pixel (8,8) starts the second tile in each direction
			Assert::AreEqual((1u << 24) | (0u << 8) | 0u, At(image, 1, 1));
			Assert::AreEqual((1u << 24) | (7u << 8) | 7u, At(image, 8, 8));
			Assert::AreEqual((1u << 24) | (8u << 8) | 8u, At(image, 10, 10));
		}
	};
}
