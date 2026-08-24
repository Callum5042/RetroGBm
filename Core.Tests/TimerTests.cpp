#include "CppUnitTest.h"
#include "MockCartridge.h"
#include "NullDisplayOutput.h"
#include "NullNetworkOutput.h"
#include "NullSoundOutput.h"

#include <RetroGBm/Timer.h>
#include <RetroGBm/Cpu.h>
#include <RetroGBm/Emulator.h>

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	namespace
	{
		std::filesystem::path FindTimerRomDirectory()
		{
			const std::filesystem::path relativePath =
				std::filesystem::path("RetroGBm") / "Resources" / "testroms" / "Gekkio" / "acceptance" / "timer";

			std::vector<std::filesystem::path> roots = {
				std::filesystem::current_path(),
				std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path()
			};

			for (std::filesystem::path root : roots)
			{
				while (!root.empty())
				{
					const std::filesystem::path candidate = root / relativePath;
					if (std::filesystem::is_directory(candidate))
					{
						return candidate;
					}

					const std::filesystem::path parent = root.parent_path();
					if (parent == root)
					{
						break;
					}
					root = parent;
				}
			}

			return {};
		}

		bool HasMooneyeSuccessRegisters(const Cpu* cpu)
		{
			return cpu->GetRegister(RegisterType8::REG_B) == 3 &&
				cpu->GetRegister(RegisterType8::REG_C) == 5 &&
				cpu->GetRegister(RegisterType8::REG_D) == 8 &&
				cpu->GetRegister(RegisterType8::REG_E) == 13 &&
				cpu->GetRegister(RegisterType8::REG_H) == 21 &&
				cpu->GetRegister(RegisterType8::REG_L) == 34;
		}

		bool RunMooneyeRom(const std::filesystem::path& romPath, std::string& failure)
		{
			NullDisplayOutput display;
			NullSoundOutput sound;
			NullNetworkOutput network;
			Emulator emulator(&display, &sound, &network);

			if (!emulator.LoadRom(romPath.string()))
			{
				failure = romPath.filename().string() + ": could not load ROM";
				return false;
			}

			constexpr int maxInstructions = 5'000'000;
			for (int i = 0; i < maxInstructions; ++i)
			{
				emulator.Tick();
				if (HasMooneyeSuccessRegisters(emulator.GetCpu()))
				{
					return true;
				}
				if (emulator.GetCpu()->ProgramCounter == 0x4AB4)
				{
					break;
				}
			}

			Cpu* cpu = emulator.GetCpu();
			std::ostringstream message;
			message << romPath.filename().string() << ": PC=0x" << std::hex << cpu->ProgramCounter
				<< " BC=0x" << cpu->GetRegister(RegisterType16::REG_BC)
				<< " DE=0x" << cpu->GetRegister(RegisterType16::REG_DE)
				<< " HL=0x" << cpu->GetRegister(RegisterType16::REG_HL);
			failure = message.str();
			return false;
		}
	}

	TEST_CLASS(TimerTests)
	{
	public:
		TEST_METHOD(Init_SetDefaults)
		{
			// Arrange
			Timer timer(nullptr);

			// Act
			timer.Init();

			// Assert
			Assert::AreEqual(0xAC00, static_cast<int>(timer.GetContext()->div));
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->tma));
			Assert::AreEqual(0xF8, static_cast<int>(timer.GetContext()->tac));
		}

		TEST_METHOD(Write_WriteToDiv_DivAlways0)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();

			// Act
			timer.Write(0xFF04, 0xA);

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->div));
		}

		TEST_METHOD(Read_OnlyHighBytes)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();

			// Set div to binary: 1111111110011001
			const_cast<TimerContext*>(timer.GetContext())->div = 0xFF99;

			// Act
			uint8_t result = timer.Read(0xFF04);

			// Assert
			Assert::AreEqual(0xFF, static_cast<int>(result));
		}

		TEST_METHOD(Tick_DivIncreasedBy1EveryTick)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();
			const_cast<TimerContext*>(timer.GetContext())->div = 0;

			// Act
			timer.Tick();

			// Assert
			Assert::AreEqual(0x1, static_cast<int>(timer.GetContext()->div));
		}

		TEST_METHOD(Tick_TmaOverflown_TimaIsZero_ResetNotYetDone)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu mockCpu(&cartridge);

			Timer timer(&mockCpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0xA);
			timer.Write(0xFF05, 0xFF);
			timer.Write(0xFF04, 0);

			// Act
			for (int i = 0; i < 16; ++i)
			{
				timer.Tick();
			}

			// Assert
			Assert::AreEqual(0x0, static_cast<int>(timer.GetContext()->tima));
		}

		TEST_METHOD(Tick_TmaOverflown_TimaIsIncremented)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu mockCpu(&cartridge);

			Timer timer(&mockCpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0xA);
			timer.Write(0xFF05, 0x5);
			timer.Write(0xFF04, 0);

			// Act
			for (int i = 0; i < 16; ++i)
			{
				timer.Tick();
			}

			// Assert
			Assert::AreEqual(0x6, static_cast<int>(timer.GetContext()->tima));
		}

		TEST_METHOD(Tick_TmaOverflown_TimaIsZero_ResetIsDone4CyclesLater)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu mockCpu(&cartridge);

			Timer timer(&mockCpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0xA);
			timer.Write(0xFF05, 0xFF);
			timer.Write(0xFF04, 0);

			// Act
			for (int i = 0; i < 20; ++i)
			{
				timer.Tick();
			}

			// Assert
			Assert::AreEqual(0xA, static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(static_cast<int>(timer.GetContext()->tma), static_cast<int>(timer.GetContext()->tima));
			Assert::AreEqual(static_cast<int>(InterruptFlag::Timer) | 0xE0, static_cast<int>(mockCpu.GetInterruptFlags()));
		}

		TEST_METHOD(Read_ReadTac_PadLast5Bits)
		{
			// Arrange
			Timer timer(nullptr);
			timer.Init();

			// Act
			uint8_t result = timer.Read(0xFF07);

			// Assert
			Assert::AreEqual(0xF8, static_cast<int>(result));
		}

		TEST_METHOD(Write_TmaNormally_DoesNotOverwriteTima)
		{
			Timer timer(nullptr);
			timer.Init();
			timer.Write(0xFF05, 0x12);

			timer.Write(0xFF06, 0x34);

			Assert::AreEqual(0x12, static_cast<int>(timer.Read(0xFF05)));
			Assert::AreEqual(0x34, static_cast<int>(timer.Read(0xFF06)));
		}

		TEST_METHOD(Write_TimaDuringOverflowDelay_CancelsReload)
		{
			MockCartridge cartridge;
			Cpu cpu(&cartridge);
			Timer timer(&cpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0x34);
			timer.Write(0xFF05, 0xFF);
			timer.Write(0xFF04, 0);
			for (int i = 0; i < 19; ++i)
			{
				timer.Tick();
			}

			timer.Write(0xFF05, 0x12);
			timer.Tick();

			Assert::AreEqual(0x12, static_cast<int>(timer.Read(0xFF05)));
			Assert::AreEqual(0xE0, static_cast<int>(cpu.GetInterruptFlags()));
		}

		TEST_METHOD(Write_TimaDuringReload_IsIgnored)
		{
			MockCartridge cartridge;
			Cpu cpu(&cartridge);
			Timer timer(&cpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0x34);
			timer.Write(0xFF05, 0xFF);
			timer.Write(0xFF04, 0);
			for (int i = 0; i < 20; ++i)
			{
				timer.Tick();
			}

			timer.Write(0xFF05, 0x12);

			Assert::AreEqual(0x34, static_cast<int>(timer.Read(0xFF05)));
		}

		TEST_METHOD(Write_TmaDuringReload_UpdatesTima)
		{
			MockCartridge cartridge;
			Cpu cpu(&cartridge);
			Timer timer(&cpu);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			timer.Write(0xFF06, 0x34);
			timer.Write(0xFF05, 0xFF);
			timer.Write(0xFF04, 0);
			for (int i = 0; i < 20; ++i)
			{
				timer.Tick();
			}

			timer.Write(0xFF06, 0x56);

			Assert::AreEqual(0x56, static_cast<int>(timer.Read(0xFF05)));
			Assert::AreEqual(0x56, static_cast<int>(timer.Read(0xFF06)));
		}

		TEST_METHOD(Write_DivCausesTimerFallingEdge_IncrementsTima)
		{
			Timer timer(nullptr);
			timer.Init();
			timer.Write(0xFF07, 0b101);
			const_cast<TimerContext*>(timer.GetContext())->div = 0b1000;

			timer.Write(0xFF04, 0);

			Assert::AreEqual(1, static_cast<int>(timer.Read(0xFF05)));
		}

		TEST_METHOD(Write_TacCausesTimerFallingEdge_IncrementsTima)
		{
			Timer timer(nullptr);
			timer.Init();
			const_cast<TimerContext*>(timer.GetContext())->div = 0b1000;
			timer.Write(0xFF07, 0b101);

			timer.Write(0xFF07, 0b100);

			Assert::AreEqual(1, static_cast<int>(timer.Read(0xFF05)));
		}

		TEST_METHOD(Mooneye_BundledTimerAcceptanceRomsPass)
		{
			const std::filesystem::path timerRomDirectory = FindTimerRomDirectory();
			Assert::IsFalse(timerRomDirectory.empty(), L"Could not find the bundled Mooneye timer ROM directory");

			std::vector<std::filesystem::path> timerRoms;
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(timerRomDirectory))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".gb")
				{
					timerRoms.push_back(entry.path());
				}
			}
			std::sort(timerRoms.begin(), timerRoms.end());

			std::vector<std::string> failures;
			for (const std::filesystem::path& timerRom : timerRoms)
			{
				std::string failure;
				if (!RunMooneyeRom(timerRom, failure))
				{
					failures.push_back(failure);
				}
			}

			if (!failures.empty())
			{
				std::ostringstream message;
				message << "Bundled Mooneye timer failures:";
				for (const std::string& failure : failures)
				{
					message << "\n  " << failure;
				}
				Logger::WriteMessage(message.str().c_str());
				Assert::Fail(L"One or more bundled Mooneye timer ROMs failed; see test output");
			}
		}
	};
}
