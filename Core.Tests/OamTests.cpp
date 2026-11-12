#include "CppUnitTest.h"
#include <RetroGBm/Oam.h>
#include <RetroGBm/TileData.h>

#include <array>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(OamTests)
	{
		using Bytes = std::array<uint8_t, Oam::Size>;

		static void SetObject(Bytes& oam, int index, uint8_t y, uint8_t x, uint8_t tile = 0, uint8_t flags = 0)
		{
			oam[index * 4] = y;
			oam[index * 4 + 1] = x;
			oam[index * 4 + 2] = tile;
			oam[index * 4 + 3] = flags;
		}

		// Fills one row of a tile with a colour index
		static void FillRow(std::vector<uint8_t>& bank, int tile, int row, uint8_t colour)
		{
			bank[tile * 16 + row * 2] = (colour & 1) ? 0xFF : 0x00;
			bank[tile * 16 + row * 2 + 1] = (colour & 2) ? 0xFF : 0x00;
		}

		static Oam::Object MakeObject(uint8_t y, uint8_t x, uint8_t tile = 0, uint8_t flags = 0)
		{
			return Oam::Object{ y, x, tile, flags };
		}

	public:
		TEST_METHOD(DecodeAttributes_ReadsEveryField)
		{
			const Oam::Attributes all = Oam::DecodeAttributes(0xFF);
			Assert::IsTrue(all.priority);
			Assert::IsTrue(all.flip_y);
			Assert::IsTrue(all.flip_x);
			Assert::AreEqual<int>(1, all.dmg_palette);
			Assert::AreEqual<int>(1, all.bank);
			Assert::AreEqual<int>(7, all.cgb_palette);

			const Oam::Attributes some = Oam::DecodeAttributes(0x25);
			Assert::IsFalse(some.priority);
			Assert::IsFalse(some.flip_y);
			Assert::IsTrue(some.flip_x);
			Assert::AreEqual<int>(0, some.dmg_palette);
			Assert::AreEqual<int>(0, some.bank);
			Assert::AreEqual<int>(5, some.cgb_palette);
		}

		TEST_METHOD(Decode_ReadsTheObjectsFourBytes)
		{
			Bytes oam = {};
			SetObject(oam, 39, 0x10, 0x20, 0x30, 0x40);

			const Oam::Object object = Oam::Decode(oam.data(), 39);
			Assert::AreEqual<int>(0x10, object.y);
			Assert::AreEqual<int>(0x20, object.x);
			Assert::AreEqual<int>(0x30, object.tile);
			Assert::AreEqual<int>(0x40, object.flags);
			Assert::AreEqual(0x20 - 8, object.ScreenX());
			Assert::AreEqual(0x10 - 16, object.ScreenY());

			Assert::AreEqual<int>(0xFE00, Oam::ObjectAddress(0));
			Assert::AreEqual<int>(0xFE9C, Oam::ObjectAddress(39));
		}

		TEST_METHOD(Height_FollowsLcdcBit2)
		{
			Assert::AreEqual(8, Oam::HeightFromLcdc(0xFB));
			Assert::AreEqual(16, Oam::HeightFromLcdc(0x04));
		}

		TEST_METHOD(TileAddress_TallObjectsIgnoreBit0)
		{
			Assert::AreEqual<int>(0x05, Oam::FirstTile(0x05, 8));
			Assert::AreEqual<int>(0x04, Oam::FirstTile(0x05, 16));
			Assert::AreEqual<int>(0x8050, Oam::TileAddress(0x05, 8));
			Assert::AreEqual<int>(0x8040, Oam::TileAddress(0x05, 16));
			Assert::AreEqual<int>(0x8FF0, Oam::TileAddress(0xFF, 8));
		}

		TEST_METHOD(IsOnLines_CoversAnyLineOfTheScreen)
		{
			Assert::IsFalse(Oam::IsOnLines(MakeObject(0, 8), 16));
			Assert::IsFalse(Oam::IsOnLines(MakeObject(8, 8), 8));
			Assert::IsTrue(Oam::IsOnLines(MakeObject(9, 8), 8));
			Assert::IsTrue(Oam::IsOnLines(MakeObject(1, 8), 16));
			Assert::IsTrue(Oam::IsOnLines(MakeObject(159, 8), 8));
			Assert::IsFalse(Oam::IsOnLines(MakeObject(160, 8), 16));
		}

		TEST_METHOD(IsVisible_NeedsXFrom1To167)
		{
			Assert::IsFalse(Oam::IsVisible(MakeObject(16, 0), 8));
			Assert::IsTrue(Oam::IsVisible(MakeObject(16, 1), 8));
			Assert::IsTrue(Oam::IsVisible(MakeObject(16, 167), 8));
			Assert::IsFalse(Oam::IsVisible(MakeObject(16, 168), 8));
			Assert::IsFalse(Oam::IsVisible(MakeObject(0, 80), 8));
		}

		TEST_METHOD(DecodeImage_FlipsXAndY)
		{
			std::vector<uint8_t> bank(TileData::BytesPerBank);

			// Tile 2: the leftmost pixel of row 0 is colour 3
			bank[2 * 16] = 0x80;
			bank[2 * 16 + 1] = 0x80;

			const std::vector<uint8_t> plain = Oam::DecodeImage(MakeObject(16, 8, 2, 0x00), 8, bank.data(), nullptr);
			Assert::AreEqual<size_t>(64, plain.size());
			Assert::AreEqual<int>(3, plain[0]);

			const std::vector<uint8_t> flip_x = Oam::DecodeImage(MakeObject(16, 8, 2, 0x20), 8, bank.data(), nullptr);
			Assert::AreEqual<int>(0, flip_x[0]);
			Assert::AreEqual<int>(3, flip_x[7]);

			const std::vector<uint8_t> flip_y = Oam::DecodeImage(MakeObject(16, 8, 2, 0x40), 8, bank.data(), nullptr);
			Assert::AreEqual<int>(0, flip_y[0]);
			Assert::AreEqual<int>(3, flip_y[7 * 8]);

			const std::vector<uint8_t> both = Oam::DecodeImage(MakeObject(16, 8, 2, 0x60), 8, bank.data(), nullptr);
			Assert::AreEqual<int>(3, both[63]);
		}

		TEST_METHOD(DecodeImage_TallObjectFlipsAsAWhole)
		{
			std::vector<uint8_t> bank(TileData::BytesPerBank);
			FillRow(bank, 4, 0, 1);
			FillRow(bank, 5, 7, 2);

			// Tile 5 uses the pair 4 and 5
			const std::vector<uint8_t> plain = Oam::DecodeImage(MakeObject(16, 8, 5, 0x00), 16, bank.data(), nullptr);
			Assert::AreEqual<size_t>(128, plain.size());
			Assert::AreEqual<int>(1, plain[0]);
			Assert::AreEqual<int>(2, plain[15 * 8]);

			// Flipped vertically, the bottom row of tile 5 comes first
			const std::vector<uint8_t> flipped = Oam::DecodeImage(MakeObject(16, 8, 5, 0x40), 16, bank.data(), nullptr);
			Assert::AreEqual<int>(2, flipped[0]);
			Assert::AreEqual<int>(1, flipped[15 * 8]);
		}

		TEST_METHOD(DecodeImage_UsesBank1OnlyWhenGiven)
		{
			std::vector<uint8_t> bank0(TileData::BytesPerBank);
			std::vector<uint8_t> bank1(TileData::BytesPerBank);
			FillRow(bank0, 0, 0, 1);
			FillRow(bank1, 0, 0, 2);

			Assert::AreEqual<int>(2, Oam::DecodeImage(MakeObject(16, 8, 0, 0x08), 8, bank0.data(), bank1.data())[0]);
			Assert::AreEqual<int>(1, Oam::DecodeImage(MakeObject(16, 8, 0, 0x00), 8, bank0.data(), bank1.data())[0]);
			Assert::AreEqual<int>(1, Oam::DecodeImage(MakeObject(16, 8, 0, 0x08), 8, bank0.data(), nullptr)[0]);
		}

		TEST_METHOD(ScanLines_DropsObjectsPastTheTenthInOamOrder)
		{
			Bytes oam = {};

			// 12 objects on lines 40-47, in reverse X order so X doesn't decide who is picked
			for (int i = 0; i < 12; ++i)
			{
				SetObject(oam, i, 56, static_cast<uint8_t>(160 - i * 8));
			}

			const Oam::LineScan scan = Oam::ScanLines(oam.data(), 8);
			Assert::AreEqual<int>(12, scan.objects[40]);
			Assert::AreEqual<int>(12, scan.objects[47]);
			Assert::AreEqual<int>(0, scan.objects[48]);
			Assert::IsTrue(scan.IsOverLimit(40));
			Assert::IsFalse(scan.IsOverLimit(48));

			for (int i = 0; i < 10; ++i)
			{
				Assert::IsTrue(scan.dropped[i].empty());
			}

			for (int i = 10; i < 12; ++i)
			{
				Assert::AreEqual<size_t>(8, scan.dropped[i].size());
				Assert::AreEqual<int>(40, scan.dropped[i].front());
				Assert::AreEqual<int>(47, scan.dropped[i].back());
			}
		}

		TEST_METHOD(ScanLines_HiddenXCountsButOffScreenYDoesNot)
		{
			Bytes oam = {};

			// Ten objects at X = 0 take every slot on lines 0-7
			for (int i = 0; i < 10; ++i)
			{
				SetObject(oam, i, 16, 0);
			}

			// Y = 0 is above the screen and takes no slot
			SetObject(oam, 10, 0, 80);

			// Visible, but too late in OAM for lines 0-7
			SetObject(oam, 11, 16, 80);

			const Oam::LineScan scan = Oam::ScanLines(oam.data(), 8);
			Assert::AreEqual<int>(11, scan.objects[0]);
			Assert::IsTrue(scan.dropped[10].empty());
			Assert::AreEqual<size_t>(8, scan.dropped[11].size());
		}

		TEST_METHOD(ScanLines_TallObjectsCoverSixteenLines)
		{
			Bytes oam = {};

			// Lines 4-19 in 8 x 16, but only lines 4-11 in 8 x 8
			SetObject(oam, 0, 20, 80);

			const Oam::LineScan tall = Oam::ScanLines(oam.data(), 16);
			Assert::AreEqual<int>(0, tall.objects[3]);
			Assert::AreEqual<int>(1, tall.objects[4]);
			Assert::AreEqual<int>(1, tall.objects[19]);
			Assert::AreEqual<int>(0, tall.objects[20]);

			const Oam::LineScan small = Oam::ScanLines(oam.data(), 8);
			Assert::AreEqual<int>(1, small.objects[11]);
			Assert::AreEqual<int>(0, small.objects[12]);

			// Partly above the screen: lines 0-7 of a tall object at Y = 8
			SetObject(oam, 0, 8, 80);
			const Oam::LineScan clipped = Oam::ScanLines(oam.data(), 16);
			Assert::AreEqual<int>(1, clipped.objects[0]);
			Assert::AreEqual<int>(1, clipped.objects[7]);
			Assert::AreEqual<int>(0, clipped.objects[8]);
		}
	};
}
