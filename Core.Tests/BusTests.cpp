#include "CppUnitTest.h"
#include "Bus.h"
#include "Cpu.h"
#include "Cartridge.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(BusTests)
	{
	public:
		TEST_METHOD(BusRead_ReadFromCartridge)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.cartridge->data.resize(0x10);
			std::fill(context.cartridge->data.begin(), context.cartridge->data.end(), 0x0);
			context.cartridge->data[0x5] = 0x30;

			// Act
			uint16_t result = ReadFromBus(&context, 0x5);

			// Assert
			Assert::AreEqual(0x30, static_cast<int>(result));
		}

		TEST_METHOD(BusRead_ReadFromVideoRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.video_ram.resize(8000);
			std::fill(context.video_ram.begin(), context.video_ram.end(), 0x0);

			uint16_t address = 0x8000;
			context.video_ram[address - 0x8000] = 0x52;

			// Act
			uint16_t result = ReadFromBus(&context, address);

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(result));
		}

		TEST_METHOD(BusRead_WriteToVideoRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.video_ram.resize(8000);
			std::fill(context.video_ram.begin(), context.video_ram.end(), 0x0);

			uint16_t address = 0x8000;
			context.video_ram[address - 0x8000] = 0x0;

			// Act
			WriteToBus(&context, address, 0x52);

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(context.video_ram[address - 0x8000]));
		}

		TEST_METHOD(BusRead_StartOfWorkRAM_ReadFromWorkRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.work_ram.resize(8000);
			std::fill(context.work_ram.begin(), context.work_ram.end(), 0x0);

			uint16_t address = 0xC000;
			context.work_ram[address - 0xC000] = 0x52;

			// Act
			uint16_t result = ReadFromBus(&context, address);

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(result));
		}

		TEST_METHOD(BusWrite_StartOfWorkRAM_WriteToWorkRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			context.work_ram.resize(8000);
			std::fill(context.work_ram.begin(), context.work_ram.end(), 0x0);

			uint16_t address = 0xC000;
			context.work_ram[address - 0xC000] = 0x0;

			// Act
			WriteToBus(&context, address, static_cast<uint8_t>(0x52));

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(context.work_ram[address - 0xC000]));
		}

		TEST_METHOD(BusWrite_StartOfHighRAM_WriteToHighRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			uint16_t address = 0xFF80;
			context.high_ram[0] = 0x0;

			// Act
			WriteToBus(&context, address, static_cast<uint8_t>(0x52));

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(context.high_ram[0]));
		}

		TEST_METHOD(BusRead_StartOfHighRAM_ReadFromHighRAM)
		{
			// Arrange
			EmulatorContext context;
			context.cartridge = std::make_unique<CartridgeInfo>();

			uint16_t address = 0xFF80;
			context.high_ram[0] = 0x52;

			// Act
			uint8_t result = ReadFromBus(&context, address);

			// Assert
			Assert::AreEqual(0x52, static_cast<int>(result));
		}
	};
}