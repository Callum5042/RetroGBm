#include "CppUnitTest.h"
#include <RetroGBm/TileData.h>
#include <RetroGBm/TileMap.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(TileMapTests)
	{
		static void AreEqual(const TileMap::Rect& expected, const TileMap::Rect& actual)
		{
			Assert::AreEqual(expected.x, actual.x);
			Assert::AreEqual(expected.y, actual.y);
			Assert::AreEqual(expected.width, actual.width);
			Assert::AreEqual(expected.height, actual.height);
		}

		// Fills a tile with one colour index
		static void FillTile(std::vector<uint8_t>& bank, int tile, uint8_t colour)
		{
			for (int row = 0; row < 8; ++row)
			{
				bank[tile * 16 + row * 2] = (colour & 1) ? 0xFF : 0x00;
				bank[tile * 16 + row * 2 + 1] = (colour & 2) ? 0xFF : 0x00;
			}
		}

	public:
		TEST_METHOD(MapSelect_FollowsLcdcBits3And6)
		{
			Assert::AreEqual<int>(0x9800, TileMap::BackgroundMap(0x00));
			Assert::AreEqual<int>(0x9C00, TileMap::BackgroundMap(0x08));
			Assert::AreEqual<int>(0x9800, TileMap::WindowMap(0x08));
			Assert::AreEqual<int>(0x9C00, TileMap::WindowMap(0x40));
		}

		TEST_METHOD(Addressing_FollowsLcdcBit4)
		{
			Assert::IsTrue(TileMap::AddressingFromLcdc(0x10) == TileMap::Addressing::Unsigned);
			Assert::IsTrue(TileMap::AddressingFromLcdc(0xEF) == TileMap::Addressing::Signed);
		}

		TEST_METHOD(TileAddress_UnsignedCountsFrom8000)
		{
			Assert::AreEqual<int>(0x8000, TileMap::TileAddress(0x00, TileMap::Addressing::Unsigned));
			Assert::AreEqual<int>(0x87F0, TileMap::TileAddress(0x7F, TileMap::Addressing::Unsigned));
			Assert::AreEqual<int>(0x8800, TileMap::TileAddress(0x80, TileMap::Addressing::Unsigned));
			Assert::AreEqual<int>(0x8FF0, TileMap::TileAddress(0xFF, TileMap::Addressing::Unsigned));
		}

		TEST_METHOD(TileAddress_SignedCountsFrom9000)
		{
			Assert::AreEqual<int>(0x9000, TileMap::TileAddress(0x00, TileMap::Addressing::Signed));
			Assert::AreEqual<int>(0x97F0, TileMap::TileAddress(0x7F, TileMap::Addressing::Signed));
			Assert::AreEqual<int>(0x8800, TileMap::TileAddress(0x80, TileMap::Addressing::Signed));
			Assert::AreEqual<int>(0x8FF0, TileMap::TileAddress(0xFF, TileMap::Addressing::Signed));
			Assert::AreEqual(383, TileMap::TileIndex(0x7F, TileMap::Addressing::Signed));
		}

		TEST_METHOD(DecodeAttributes_SplitsTheByte)
		{
			const TileMap::Attributes none = TileMap::DecodeAttributes(0x00);
			Assert::AreEqual<int>(0, none.palette);
			Assert::AreEqual<int>(0, none.bank);
			Assert::IsFalse(none.flip_x || none.flip_y || none.priority);

			const TileMap::Attributes all = TileMap::DecodeAttributes(0xFF);
			Assert::AreEqual<int>(7, all.palette);
			Assert::AreEqual<int>(1, all.bank);
			Assert::IsTrue(all.flip_x && all.flip_y && all.priority);

			const TileMap::Attributes some = TileMap::DecodeAttributes(0x2D);
			Assert::AreEqual<int>(5, some.palette);
			Assert::AreEqual<int>(1, some.bank);
			Assert::IsTrue(some.flip_x);
			Assert::IsFalse(some.flip_y || some.priority);
		}

		TEST_METHOD(Decode_PlacesTilesFromTheChosenAddressing)
		{
			std::vector<uint8_t> bank(TileData::BytesPerBank);
			FillTile(bank, 1, 1);
			FillTile(bank, 257, 2);

			std::vector<uint8_t> ids(TileMap::Entries);
			ids[33] = 1; // column 1, row 1

			const TileMap::Image unsigned_image = TileMap::Decode(ids.data(), nullptr, bank.data(), nullptr, TileMap::Addressing::Unsigned);
			Assert::AreEqual<size_t>(TileMap::Width * TileMap::Height, unsigned_image.colours.size());
			Assert::AreEqual<int>(1, unsigned_image.colours[8 * TileMap::Width + 8]);
			Assert::AreEqual<int>(1, unsigned_image.colours[15 * TileMap::Width + 15]);
			Assert::AreEqual<int>(0, unsigned_image.colours[16 * TileMap::Width + 16]);
			Assert::AreEqual<int>(0, unsigned_image.palettes[8 * TileMap::Width + 8]);

			const TileMap::Image signed_image = TileMap::Decode(ids.data(), nullptr, bank.data(), nullptr, TileMap::Addressing::Signed);
			Assert::AreEqual<int>(2, signed_image.colours[8 * TileMap::Width + 8]);
		}

		TEST_METHOD(Decode_AppliesAttributes)
		{
			// Tile 0 in bank 0 is colour 1. Tile 0 in bank 1 has only its top-left pixel set to colour 3.
			std::vector<uint8_t> bank0(TileData::BytesPerBank);
			std::vector<uint8_t> bank1(TileData::BytesPerBank);
			FillTile(bank0, 0, 1);
			bank1[0] = 0x80;
			bank1[1] = 0x80;

			std::vector<uint8_t> ids(TileMap::Entries);
			std::vector<uint8_t> attributes(TileMap::Entries);
			attributes[0] = 0x08 | 0x20 | 0x40 | 0x03; // bank 1, flip both ways, palette 3

			const TileMap::Image image = TileMap::Decode(ids.data(), attributes.data(), bank0.data(), bank1.data(), TileMap::Addressing::Unsigned);

			// The set pixel moves to the bottom right of the entry
			Assert::AreEqual<int>(0, image.colours[0]);
			Assert::AreEqual<int>(3, image.colours[7 * TileMap::Width + 7]);
			Assert::AreEqual<int>(3, image.palettes[0]);

			// The next entry has no attributes, so it uses bank 0
			Assert::AreEqual<int>(1, image.colours[8]);
			Assert::AreEqual<int>(0, image.palettes[8]);
		}

		TEST_METHOD(Wrap_InsideTheMapIsOnePiece)
		{
			const std::vector<TileMap::Rect> pieces = TileMap::Wrap({ 10, 20, 160, 144 });
			Assert::AreEqual<size_t>(1, pieces.size());
			AreEqual({ 10, 20, 160, 144 }, pieces[0]);
		}

		TEST_METHOD(Wrap_SplitsAtBothEdges)
		{
			const std::vector<TileMap::Rect> pieces = TileMap::Wrap({ 176, 120, 160, 144 });
			Assert::AreEqual<size_t>(4, pieces.size());
			AreEqual({ 176, 120, 80, 136 }, pieces[0]);
			AreEqual({ 0, 120, 80, 136 }, pieces[1]);
			AreEqual({ 176, 0, 80, 8 }, pieces[2]);
			AreEqual({ 0, 0, 80, 8 }, pieces[3]);
		}

		TEST_METHOD(Wrap_EmptyIsNothing)
		{
			Assert::IsTrue(TileMap::Wrap({ 0, 0, 0, 10 }).empty());
		}

		TEST_METHOD(Window_OffOrOutOfViewIsEmpty)
		{
			Assert::IsTrue(TileMap::WindowOnScreen(0x00, 7, 0).IsEmpty());
			Assert::IsTrue(TileMap::WindowOnScreen(0x20, 167, 0).IsEmpty());
			Assert::IsTrue(TileMap::WindowOnScreen(0x20, 7, 144).IsEmpty());
			Assert::IsTrue(TileMap::WindowOnMap(0x00, 7, 0).IsEmpty());
		}

		TEST_METHOD(Window_CoversFromWxMinus7AndWy)
		{
			AreEqual({ 0, 128, 160, 16 }, TileMap::WindowOnScreen(0x20, 7, 128));
			AreEqual({ 0, 0, 160, 16 }, TileMap::WindowOnMap(0x20, 7, 128));

			AreEqual({ 80, 0, 80, 144 }, TileMap::WindowOnScreen(0x20, 87, 0));
			AreEqual({ 0, 0, 80, 144 }, TileMap::WindowOnMap(0x20, 87, 0));

			// Below 7, the window's first columns are off the left of the screen
			AreEqual({ 0, 0, 160, 144 }, TileMap::WindowOnScreen(0x20, 3, 0));
			AreEqual({ 4, 0, 160, 144 }, TileMap::WindowOnMap(0x20, 3, 0));
		}

		TEST_METHOD(ScreenPosition_FollowsScrollAndWraps)
		{
			const auto origin = TileMap::ScreenPosition(0, 0, 0, 0);
			Assert::IsTrue(origin.has_value() && origin->x == 0 && origin->y == 0);

			const auto scrolled = TileMap::ScreenPosition(24, 16, 176, 120);
			Assert::IsTrue(scrolled.has_value() && scrolled->x == 16 && scrolled->y == 8);

			// Map column 2 is 96 pixels across once the view wraps past the right edge
			const auto wrapped = TileMap::ScreenPosition(2, 0, 176, 120);
			Assert::IsTrue(wrapped.has_value() && wrapped->x == 96 && wrapped->y == 136);

			// Cut by the left edge
			const auto cut = TileMap::ScreenPosition(0, 0, 4, 0);
			Assert::IsTrue(cut.has_value() && cut->x == -4);

			Assert::IsFalse(TileMap::ScreenPosition(20, 0, 0, 0).has_value());
			Assert::IsFalse(TileMap::ScreenPosition(0, 18, 0, 0).has_value());
		}
	};
}
