#include "CppUnitTest.h"
#include "NullDisplayOutput.h"
#include "NullSoundOutput.h"
#include "NullNetworkOutput.h"

#include <RetroGBm/Cheats.h>
#include <RetroGBm/Emulator.h>
#include <RetroGBm/Ram.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheatsTests
{
	TEST_CLASS(CheatsTests)
	{
	public:
		TEST_METHOD(ParseGamesharkCode_ReturnsToken)
		{
			// Act
			GamesharkToken token = ParseGamesharkCode("01FB04D2");

			// Assert
			Assert::AreEqual(static_cast<uint8_t>(0x01), token.bank);
			Assert::AreEqual(static_cast<uint8_t>(0xFB), token.value);
			Assert::AreEqual(static_cast<uint16_t>(0xD204), token.address);
		}

		TEST_METHOD(Tick_EnabledCheat_AppliedEachFrame)
		{
			// Arrange - ROM only cartridge filled with NOPs
			std::vector<uint8_t> rom(0x8000, 0x00);

			NullDisplayOutput display_output;
			NullSoundOutput sound_output;
			NullNetworkOutput network_output;
			Emulator emulator(&display_output, &sound_output, &network_output);
			emulator.SetFramePacingEnabled(false);
			emulator.LoadRom(rom);

			emulator.SetGamesharkCodes({ { "Test", { "01FB04D2" }, true } });

			// Act - run more than one frame (70224 dots)
			for (int i = 0; i < 20000; ++i)
			{
				emulator.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<uint8_t>(0xFB), emulator.GetRam()->ReadWorkRam(0xD204));
		}

		TEST_METHOD(Tick_DisabledCheat_NotApplied)
		{
			// Arrange - ROM only cartridge filled with NOPs
			std::vector<uint8_t> rom(0x8000, 0x00);

			NullDisplayOutput display_output;
			NullSoundOutput sound_output;
			NullNetworkOutput network_output;
			Emulator emulator(&display_output, &sound_output, &network_output);
			emulator.SetFramePacingEnabled(false);
			emulator.LoadRom(rom);

			emulator.SetGamesharkCodes({ { "Test", { "01FB04D2" }, false } });

			// Act - run more than one frame (70224 dots)
			for (int i = 0; i < 20000; ++i)
			{
				emulator.Tick();
			}

			// Assert
			Assert::AreEqual(static_cast<uint8_t>(0x00), emulator.GetRam()->ReadWorkRam(0xD204));
		}
	};
}
