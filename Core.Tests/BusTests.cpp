#include "CppUnitTest.h"
#include "NullDisplayOutput.h"
#include "NullNetworkOutput.h"
#include "NullSoundOutput.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/Ram.h>

#include <filesystem>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	namespace
	{
		// ROM only cartridge; header $0143 = $80 marks a CGB game. $1000-$109F holds a
		// recognisable pattern for OAM DMA to copy.
		std::vector<uint8_t> MakeRom(bool cgb)
		{
			std::vector<uint8_t> rom(0x8000, 0x00);
			rom[0x0143] = cgb ? 0x80 : 0x00;
			for (int i = 0; i < 0xA0; ++i)
			{
				rom[0x1000 + i] = uint8_t(i ^ 0x5A);
			}

			return rom;
		}

		struct TestEmulator
		{
			NullDisplayOutput display_output;
			NullSoundOutput sound_output;
			NullNetworkOutput network_output;
			Emulator emulator{ &display_output, &sound_output, &network_output };

			explicit TestEmulator(bool cgb)
			{
				emulator.SetFramePacingEnabled(false);
				emulator.LoadRom(MakeRom(cgb));
			}
		};
	}

	TEST_CLASS(BusTests)
	{
	public:
		TEST_METHOD(UndocumentedRegisters_CgbGame_InitialValuesAndWritableBits)
		{
			// Arrange
			TestEmulator test(true);
			Emulator& emulator = test.emulator;

			// Assert - initial values
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadIO(0xFF72)));
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadIO(0xFF73)));
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadIO(0xFF74)));
			Assert::AreEqual(0x8F, static_cast<int>(emulator.ReadIO(0xFF75)));

			// Act
			emulator.WriteIO(0xFF72, 0x12);
			emulator.WriteIO(0xFF73, 0x34);
			emulator.WriteIO(0xFF74, 0x56);
			emulator.WriteIO(0xFF75, 0x50);

			// Assert - $FF75 keeps only bits 4-6 and reads the rest as 1
			Assert::AreEqual(0x12, static_cast<int>(emulator.ReadIO(0xFF72)));
			Assert::AreEqual(0x34, static_cast<int>(emulator.ReadIO(0xFF73)));
			Assert::AreEqual(0x56, static_cast<int>(emulator.ReadIO(0xFF74)));
			Assert::AreEqual(0xDF, static_cast<int>(emulator.ReadIO(0xFF75)));

			emulator.WriteIO(0xFF75, 0x8F);
			Assert::AreEqual(0x8F, static_cast<int>(emulator.ReadIO(0xFF75)));
		}

		TEST_METHOD(UndocumentedRegisters_DmgGame_FF74LockedOthersWritable)
		{
			// Arrange
			TestEmulator test(false);
			Emulator& emulator = test.emulator;

			// Act
			emulator.WriteIO(0xFF72, 0x12);
			emulator.WriteIO(0xFF73, 0x34);
			emulator.WriteIO(0xFF74, 0x56);
			emulator.WriteIO(0xFF75, 0x70);

			// Assert
			Assert::AreEqual(0x12, static_cast<int>(emulator.ReadIO(0xFF72)));
			Assert::AreEqual(0x34, static_cast<int>(emulator.ReadIO(0xFF73)));
			Assert::AreEqual(0xFF, static_cast<int>(emulator.ReadIO(0xFF74)));
			Assert::AreEqual(0xFF, static_cast<int>(emulator.ReadIO(0xFF75)));
		}

		TEST_METHOD(UndocumentedRegisters_SaveState_RestoredAndOlderStatesLoad)
		{
			// Arrange
			TestEmulator test(true);
			Emulator& emulator = test.emulator;
			auto path = std::filesystem::temp_directory_path() / "RetroGBm-undocumented-registers.state";
			emulator.WriteIO(0xFF72, 0x12);
			emulator.WriteIO(0xFF75, 0x30);
			emulator.SaveState(path.string());

			// Act - load the state over different values
			emulator.WriteIO(0xFF72, 0x99);
			emulator.WriteIO(0xFF75, 0x00);
			emulator.LoadState(path.string());

			// Assert
			Assert::AreEqual(0x12, static_cast<int>(emulator.ReadIO(0xFF72)));
			Assert::AreEqual(0xBF, static_cast<int>(emulator.ReadIO(0xFF75)));

			// Act - a state saved before the registers were added ends 4 bytes earlier
			std::filesystem::resize_file(path, std::filesystem::file_size(path) - 4);
			emulator.LoadState(path.string());

			// Assert - they fall back to their initial values
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadIO(0xFF72)));
			Assert::AreEqual(0x8F, static_cast<int>(emulator.ReadIO(0xFF75)));
			std::filesystem::remove(path);
		}

		TEST_METHOD(OamDma_FromRom_CpuReadsOnCartridgeBusGetTheDmaByte)
		{
			// Arrange
			TestEmulator test(true);
			Emulator& emulator = test.emulator;
			emulator.WriteBus(0xC000, 0x42);
			emulator.WriteBus(0xFF80, 0x24);

			// Act - start a transfer from $1000; it copies its first byte two M-cycles later
			emulator.WriteIO(0xFF46, 0x10);
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadBus(0x0200)));
			emulator.Cycle(2);

			// Assert - ROM reads see the byte being copied, other buses are unaffected
			Assert::AreEqual(0x00 ^ 0x5A, static_cast<int>(emulator.ReadBus(0x0200)));
			Assert::AreEqual(0x00 ^ 0x5A, static_cast<int>(emulator.ReadBus(0x7FFF)));
			Assert::AreEqual(0x42, static_cast<int>(emulator.ReadBus(0xC000)));
			Assert::AreEqual(0x24, static_cast<int>(emulator.ReadBus(0xFF80)));

			emulator.Cycle(1);
			Assert::AreEqual(0x01 ^ 0x5A, static_cast<int>(emulator.ReadBus(0x0200)));

			// Act - finish the 160 byte transfer
			emulator.Cycle(159);

			// Assert - reads are normal again, and DMA copied the real ROM bytes
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadBus(0x0200)));
			Assert::AreEqual(0x00 ^ 0x5A, static_cast<int>(emulator.GetPpu()->GetOam()[0x00]));
			Assert::AreEqual(0x9F ^ 0x5A, static_cast<int>(emulator.GetPpu()->GetOam()[0x9F]));
		}

		TEST_METHOD(OamDma_FromWorkRam_CpuReadsOnWorkRamBusGetTheDmaByte)
		{
			// Arrange
			TestEmulator test(true);
			Emulator& emulator = test.emulator;
			emulator.WriteBus(0xC005, 0x77);
			emulator.WriteBus(0xD123, 0x11);

			// Act - run until byte 5 of a transfer from $C000 is being copied
			emulator.WriteIO(0xFF46, 0xC0);
			emulator.Cycle(2 + 5);

			// Assert - WRAM and its echo see the DMA byte, the cartridge bus does not
			Assert::AreEqual(0x77, static_cast<int>(emulator.ReadBus(0xD123)));
			Assert::AreEqual(0x77, static_cast<int>(emulator.ReadBus(0xE000)));
			Assert::AreEqual(0x00, static_cast<int>(emulator.ReadBus(0x0200)));
		}
	};
}
