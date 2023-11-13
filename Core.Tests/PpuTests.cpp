#include "CppUnitTest.h"
#include <Display.h>
#include <Ppu.h>
#include <Cpu.h>
#include <Emulator.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	class NullBus : public IBus
	{
	public:
		NullBus() = default;

		uint8_t ReadBus(uint16_t address) override { return 0; }
		void WriteBus(uint16_t address, uint8_t value) override {}
	};

	TEST_CLASS(PpuTests)
	{
	public:
		TEST_METHOD(Init_StartsInOAM_Has0DotTicks_RamAndBufferInitialisedAndResizedCorrectly)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);

			// Act
			ppu.Init();

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.GetContext()->dot_ticks));

			Assert::AreEqual(0x2000, static_cast<int>(ppu.GetContext()->video_ram.size()));
			Assert::AreEqual(static_cast<int>(144 * 160), static_cast<int>(ppu.GetContext()->video_buffer.size()));
			Assert::AreEqual(static_cast<int>(40), static_cast<int>(ppu.GetContext()->oam_ram.size()));
		}

		TEST_METHOD(Tick_80Ticks_EnterPixelTransferMode)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 80; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::PixelTransfer), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(80, static_cast<int>(ppu.GetContext()->dot_ticks));
		}

		TEST_METHOD(Tick_456Ticks_EnterOamMode_DotTicksReset_IncreaseDisplayLyRegister)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 456; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.GetContext()->dot_ticks));
			Assert::AreEqual(1, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(Tick_65664Ticks_EnterVBlankMode)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			// Act
			for (int i = 0; i < 65664; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::VBlank), static_cast<int>(display.GetLcdMode()));
			Assert::IsTrue(static_cast<bool>(cpu.GetInterruptFlags() & InterruptFlag::VBlank));

			Assert::AreEqual(0, static_cast<int>(ppu.GetContext()->dot_ticks));
			Assert::AreEqual(144, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(Tick_IsInVBlankMode_TickFor4560Dots_Resets)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			const_cast<DisplayContext*>(display.GetContext())->ly = 144;
			display.SetLcdMode(LcdMode::VBlank);

			// Act
			for (int i = 0; i < 4560; ++i)
			{
				ppu.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<int>(LcdMode::OAM), static_cast<int>(display.GetLcdMode()));
			Assert::AreEqual(0, static_cast<int>(ppu.GetContext()->dot_ticks));
			Assert::AreEqual(0, static_cast<int>(display.GetContext()->ly));
		}

		TEST_METHOD(OamWrite_PopulatesOam)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;

			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();

			const_cast<DisplayContext*>(display.GetContext())->ly = 144;
			display.SetLcdMode(LcdMode::VBlank);

			// Act
			OamData data = {};
			data.position_y = 20;
			data.position_x = 10;
			data.tile_id = 5;
			data.gcb_palette = 7;
			data.bank = false;
			data.dmg_palette = false;
			data.flip_x = true;
			data.flip_y = true;
			data.priority = false;

			uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);
			ppu.WriteOam(0xFE00, ptr[0]);
			ppu.WriteOam(0xFE01, ptr[1]);
			ppu.WriteOam(0xFE02, ptr[2]);
			ppu.WriteOam(0xFE03, ptr[3]);

			// Assert
			Assert::AreEqual(data.position_y, ppu.ReadOam(0xFE00));
			Assert::AreEqual(data.position_x, ppu.ReadOam(0xFE01));
			Assert::AreEqual(data.tile_id, ppu.ReadOam(0xFE02));

			OamData result = ppu.GetContext()->oam_ram[0];
			Assert::AreEqual(data.position_y, result.position_y);
			Assert::AreEqual(data.position_x, result.position_x);
			Assert::AreEqual(data.tile_id, result.tile_id);
			Assert::IsFalse(result.bank);
			Assert::IsFalse(result.dmg_palette);
			Assert::IsTrue(result.flip_x);
			Assert::IsTrue(result.flip_y);
			Assert::IsFalse(result.priority);
		}

		TEST_METHOD(Tick_OamSort_OrderByPositionX)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;
			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();
			const_cast<DisplayContext*>(display.GetContext())->ly = 0;

			OamData data1 = {};
			data1.position_y = 16;
			data1.position_x = 10;

			OamData data2 = {};
			data2.position_y = 16;
			data2.position_x = 5;

			OamData data3 = {};
			data3.position_y = 16;
			data3.position_x = 20;

			WriteToOam(&ppu, 0xFE00 + (sizeof(OamData) * 0), data1);
			WriteToOam(&ppu, 0xFE00 + (sizeof(OamData) * 1), data2);
			WriteToOam(&ppu, 0xFE00 + (sizeof(OamData) * 2), data3);

			// Act
			ppu.Tick();

			// Assert
			Assert::AreEqual(3, static_cast<int>(ppu.GetContext()->objects_per_line.size()));

			Assert::AreEqual(data2.position_x, ppu.GetContext()->objects_per_line[0].position_x);
			Assert::AreEqual(data1.position_x, ppu.GetContext()->objects_per_line[1].position_x);
			Assert::AreEqual(data3.position_x, ppu.GetContext()->objects_per_line[2].position_x);
		}

		TEST_METHOD(Tick_OamSort_LimitTo10PerRow)
		{
			// Arrange
			Cpu cpu;
			Display display;
			NullBus bus;
			Ppu ppu(&bus, &cpu, &display);
			ppu.Init();
			const_cast<DisplayContext*>(display.GetContext())->ly = 0;

			for (int i = 0; i < 20; ++i)
			{
				OamData data1 = {};
				data1.position_y = 16;
				data1.position_x = i + 1;

				WriteToOam(&ppu, static_cast<uint16_t>(0xFE00 + (sizeof(OamData) * i)), data1);
			}

			// Act
			ppu.Tick();

			// Assert
			Assert::AreEqual(10, static_cast<int>(ppu.GetContext()->objects_per_line.size()));
			Assert::AreEqual(10, static_cast<int>(ppu.GetContext()->objects_per_line[9].position_x));
		}

	private:
		void WriteToOam(Ppu* ppu, uint16_t address, OamData data)
		{
			uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);
			ppu->WriteOam(address + 0, ptr[0]);
			ppu->WriteOam(address + 1, ptr[1]);
			ppu->WriteOam(address + 2, ptr[2]);
			ppu->WriteOam(address + 3, ptr[3]);
		}
	};
}