#include "CppUnitTest.h"
#include "MockCartridge.h"

#include <RetroGBm/Apu.h>
#include <RetroGBm/Timer.h>
#include <RetroGBm/Cpu.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(ApuTests)
	{
	public:
		/// 
		/// Turning on channels
		/// 

		TEST_METHOD(Write_Channel1_NR14_TurnOnChannel1)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF12, 0xF8);

			// Act
			apu.Write(0xFF14, 0xC0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF1, static_cast<int>(result));
		}

		TEST_METHOD(Write_Channel1_NR24_TurnOnChannel2)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF17, 0xF8);

			// Act
			apu.Write(0xFF19, 0xC0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF3, static_cast<int>(result));
		}

		TEST_METHOD(Write_Channel3_NR34_TurnOnChannel3)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF1A, 0x80);

			// Act
			apu.Write(0xFF1E, 0xC0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF5, static_cast<int>(result));
		}

		TEST_METHOD(Write_Channel4_NR44_TurnOnChannel4)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF21, 0x80);

			// Act
			apu.Write(0xFF23, 0xC0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF9, static_cast<int>(result));
		}

		///
		/// Channels are on, turning off by DAC off
		/// 

		TEST_METHOD(Write_Channel1_NR12_TurnOffChannel1)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF12, 0x80);
			apu.Write(0xFF14, 0xC0);

			// Act
			apu.Write(0xFF12, 0x0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF0, static_cast<int>(result));
		}

		TEST_METHOD(Write_Channel2_NR22_TurnOffChannel2)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF22, 0x80);
			apu.Write(0xFF24, 0xC0);

			// Act
			apu.Write(0xFF22, 0x0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF1, static_cast<int>(result));
		}

		TEST_METHOD(Write_Channel3_NR30_TurnOffChannel3)
		{
			// Arrange
			MockCartridge cartridge;
			Cpu _cpu(&cartridge);
			Timer timer(&_cpu);
			Apu apu(&timer);

			apu.Write(0xFF1A, 0x80);
			apu.Write(0xFF1E, 0xC0);

			// Act
			apu.Write(0xFF1A, 0x0);

			// Assert
			uint8_t result = apu.Read(0xFF26);
			Assert::AreEqual(0xF1, static_cast<int>(result));
		}
	};
}