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

		TEST_METHOD(ParseGamesharkCode_HighBankByte_ReturnsToken)
		{
			// Act
			GamesharkToken token = ParseGamesharkCode("910730D2");

			// Assert
			Assert::AreEqual(static_cast<uint8_t>(0x91), token.bank);
			Assert::AreEqual(static_cast<uint8_t>(0x07), token.value);
			Assert::AreEqual(static_cast<uint16_t>(0xD230), token.address);
		}

		TEST_METHOD(ValidateGamesharkCodes_SingleCode_IsValid)
		{
			// Act
			GamesharkValidation result = ValidateGamesharkCodes("01FB04D2");

			// Assert
			Assert::IsTrue(result.valid);
			Assert::AreEqual(static_cast<size_t>(1), result.codes.size());
			Assert::AreEqual(std::string("01FB04D2"), result.codes[0]);
			Assert::AreEqual(static_cast<size_t>(0), result.error_line);
		}

		TEST_METHOD(ValidateGamesharkCodes_CrlfAndLf_SplitsLines)
		{
			// Act
			GamesharkValidation crlf = ValidateGamesharkCodes("019947D3\r\n019948D3\r\n019949D3");
			GamesharkValidation lf = ValidateGamesharkCodes("019947D3\n019948D3\n019949D3");

			// Assert
			std::vector<std::string> expected = { "019947D3", "019948D3", "019949D3" };
			Assert::IsTrue(crlf.valid);
			Assert::IsTrue(expected == crlf.codes);
			Assert::IsTrue(lf.valid);
			Assert::IsTrue(expected == lf.codes);
		}

		TEST_METHOD(ValidateGamesharkCodes_BlankLinesAndWhitespace_Ignored)
		{
			// Act
			GamesharkValidation result = ValidateGamesharkCodes("\r\n  01fb04d2  \r\n\r\n\t0199 47D3\r\n");

			// Assert
			Assert::IsTrue(result.valid);
			Assert::AreEqual(static_cast<size_t>(2), result.codes.size());
			Assert::AreEqual(std::string("01FB04D2"), result.codes[0]);
			Assert::AreEqual(std::string("019947D3"), result.codes[1]);
		}

		TEST_METHOD(ValidateGamesharkCodes_Empty_IsInvalidWithoutErrorLine)
		{
			// Act
			GamesharkValidation empty = ValidateGamesharkCodes("");
			GamesharkValidation blank = ValidateGamesharkCodes(" \r\n\t\r\n");

			// Assert
			Assert::IsFalse(empty.valid);
			Assert::AreEqual(static_cast<size_t>(0), empty.error_line);
			Assert::IsTrue(empty.codes.empty());
			Assert::IsFalse(blank.valid);
			Assert::AreEqual(static_cast<size_t>(0), blank.error_line);
		}

		TEST_METHOD(ValidateGamesharkCodes_TooShort_ReportsLine)
		{
			// Act
			GamesharkValidation result = ValidateGamesharkCodes("01FF16D0\r\n 01FF16D \r\n01FF17D0");

			// Assert
			Assert::IsFalse(result.valid);
			Assert::AreEqual(static_cast<size_t>(2), result.error_line);
			Assert::AreEqual(std::string("01FF16D"), result.error_text);
			Assert::IsTrue(result.codes.empty());
		}

		TEST_METHOD(ValidateGamesharkCodes_TooLong_ReportsLine)
		{
			// Act
			GamesharkValidation result = ValidateGamesharkCodes("01FF16D00");

			// Assert
			Assert::IsFalse(result.valid);
			Assert::AreEqual(static_cast<size_t>(1), result.error_line);
			Assert::AreEqual(std::string("01FF16D00"), result.error_text);
		}

		TEST_METHOD(ValidateGamesharkCodes_NonHex_ReportsLineCountingBlankLines)
		{
			// Act
			GamesharkValidation result = ValidateGamesharkCodes("01FB04D2\r\n\r\n01FG04D2");

			// Assert
			Assert::IsFalse(result.valid);
			Assert::AreEqual(static_cast<size_t>(3), result.error_line);
			Assert::AreEqual(std::string("01FG04D2"), result.error_text);
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
