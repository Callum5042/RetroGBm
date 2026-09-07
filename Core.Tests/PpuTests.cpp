#include "CppUnitTest.h"
#include "VideoTestFixture.h"
#include "NullSoundOutput.h"
#include "NullNetworkOutput.h"
#include <RetroGBm/Emulator.h>
#include <RetroGBm/Dma.h>
#include <RetroGBm/SaveStateHeader.h>
#include <sstream>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace VideoTests;
namespace CoreTests
{
TEST_CLASS(PpuTests)
{
public:
    TEST_METHOD(ModesFrameLengthAndInterruptEdges)
    {
        Fixture f; f.cpu.SetInterrupt(0); f.display.Write(0xFF41, 0x20);
        Assert::AreEqual(2, int(f.cpu.GetInterruptFlags() & 2)); f.cpu.SetInterrupt(0); f.Dots(79);
        Assert::AreEqual(2, int(f.display.GetLcdMode())); Assert::AreEqual(0, int(f.cpu.GetInterruptFlags() & 31));
        f.Dots(1); Assert::AreEqual(3, int(f.display.GetLcdMode())); f.Dots(172);
        Assert::AreEqual(0, int(f.display.GetLcdMode())); f.Dots(456 * 144 - 252);
        Assert::AreEqual(144, int(f.display.Read(0xFF44))); Assert::AreEqual(1, f.output.frames);
        Assert::AreEqual(1, int(f.cpu.GetInterruptFlags() & 1)); f.Dots(456 * 10);
        Assert::AreEqual(0, int(f.display.Read(0xFF44))); Assert::AreEqual(0, int(f.ppu.GetDot()));
    }
    TEST_METHOD(StatSourcesShareOneInterruptLine)
    {
        Fixture f; f.display.Write(0xFF41, 0x60); f.cpu.SetInterrupt(0); f.Dots(80);
        Assert::AreEqual(0, int(f.cpu.GetInterruptFlags() & 2));
        f.display.Write(0xFF45, 1); f.display.Write(0xFF45, 0);
        Assert::AreEqual(2, int(f.cpu.GetInterruptFlags() & 2));
        f.Dots(456 * 153 + 4 - 80); Assert::AreEqual(0, int(f.display.Read(0xFF44)));
    }
    TEST_METHOD(ScrollAndWindowExtendTransfer)
    {
        for (int scroll = 0; scroll < 8; ++scroll)
        {
            Fixture f; f.display.Write(0xFF43, uint8_t(scroll)); f.Dots(251 + scroll);
            Assert::AreEqual(3, int(f.display.GetLcdMode())); f.Dots(1); Assert::AreEqual(0, int(f.display.GetLcdMode()));
        }
        Fixture f; f.Off(); f.display.Write(0xFF4B, 87); f.On(0xB1); f.Dots(456 + 257);
        Assert::AreEqual(3, int(f.display.GetLcdMode())); f.Dots(1); Assert::AreEqual(0, int(f.display.GetLcdMode()));
    }
    TEST_METHOD(BankedMemoryAndCpuRestrictions)
    {
        Fixture f; f.Off(); f.ppu.WriteVideoRam(0x8000, 12); f.ppu.SetVideoRamBank(1); f.ppu.WriteVideoRam(0x8000, 34);
        Assert::AreEqual(12, int(f.ppu.ReadVideoRam(0x8000, 0))); Assert::AreEqual(34, int(f.ppu.ReadVideoRam(0x8000)));
        f.On(); f.Dots(456); f.ppu.WriteOam(0xFE00, 50); f.ppu.WriteOamDma(0, 16);
        Assert::AreEqual(255, int(f.ppu.ReadOam(0xFE00))); f.Dots(80);
        f.ppu.WriteVideoRam(0x8000, 99); Assert::AreEqual(255, int(f.ppu.ReadVideoRam(0x8000)));
        f.ppu.WriteVideoRamDma(0x8000, 56); f.Off(); Assert::AreEqual(56, int(f.ppu.ReadVideoRam(0x8000)));
        Assert::AreEqual(16, int(f.ppu.ReadOam(0xFE00)));
    }
    TEST_METHOD(CgbAttributesBankFlipAndPalette)
    {
        Fixture f; f.Off(); f.Tile(0, 0x80, 0, 1); f.Palette(false, 2, 1, 31);
        f.ppu.SetVideoRamBank(1); f.ppu.WriteVideoRam(0x9800, 0x2A); f.ppu.SetVideoRamBank(0);
        f.On(); f.Dots(456 * 144); Assert::AreEqual(0xFF0000FFu, f.Pixel(7)); Assert::AreEqual(0xFFFFFFFFu, f.Pixel(0));
    }
    TEST_METHOD(WindowAndSignedTiles)
    {
        Fixture f(false); f.Off(); f.display.Write(0xFF47, 0xE4);
        for (int i = 0; i < 16; ++i) f.ppu.WriteVideoRam(0x9000 + i, 255);
        f.display.Write(0xFF4B, 87); f.display.Write(0xFF4A, 1); f.ppu.WriteVideoRam(0x9C00, 1);
        f.On(0xE1); f.Dots(456 * 144);
        Assert::AreEqual(0xFF000000u, f.Pixel(79, 1)); Assert::AreEqual(0xFFFFFFFFu, f.Pixel(80, 1));
        Assert::AreEqual(0xFF000000u, f.Pixel(80, 0));
    }
    TEST_METHOD(ObjectTransparencyAndBackgroundPriority)
    {
        for (int master = 0; master < 2; ++master)
        {
            Fixture f; f.Off(); f.Tile(0, 255, 0); f.Tile(1, 0, 0x80);
            f.Palette(false, 0, 1, 31); f.Palette(true, 0, 2, 31 << 5); f.Object(0, 8, 16, 1, 0x80);
            f.On(uint8_t(0x92 | master)); f.Dots(456 * 144);
            Assert::AreEqual(master ? 0xFF0000FFu : 0xFF00FF00u, f.Pixel(0));
            Assert::AreEqual(0xFF0000FFu, f.Pixel(1));
        }
    }
    TEST_METHOD(ObjectOrderingAndTenObjectLimit)
    {
        for (int coordinate = 0; coordinate < 2; ++coordinate)
        {
            Fixture f; f.Off(); f.Tile(1, 255, 0); f.Tile(2, 0, 255);
            f.Palette(true, 0, 1, 31); f.Palette(true, 0, 2, 31 << 5);
            f.Object(0, 12, 16, 1); f.Object(1, 8, 16, 2); f.display.SetObjectPriorityMode(uint8_t(coordinate));
            f.On(0x93); f.Dots(456 * 144); Assert::AreEqual(coordinate ? 0xFF00FF00u : 0xFF0000FFu, f.Pixel(4));
        }
        Fixture f; f.Off(); f.Tile(1, 255, 255); f.Palette(true, 0, 3, 0);
        for (int i = 0; i < 10; ++i) f.Object(i, 0, 16, 1);
        f.Object(10, 8, 16, 1); f.On(0x93); f.Dots(456 * 144); Assert::AreEqual(0xFFFFFFFFu, f.Pixel(0));
    }
    TEST_METHOD(TallSpriteVerticalFlipAndBank)
    {
        Fixture f; f.Off(); f.Tile(2, 255, 0, 1); f.Tile(3, 0, 255, 1);
        f.Palette(true, 0, 2, 31); f.Object(0, 8, 16, 3, 0x48); f.On(0x97); f.Dots(456 * 144);
        Assert::AreEqual(0xFF0000FFu, f.Pixel(0));
    }
    TEST_METHOD(StateContinuationInEveryModeAndMidFetch)
    {
        auto path = std::filesystem::temp_directory_path() / "RetroGBm-video-roundtrip.state";
        for (int dots : { 39, 83, 95, 123, 300, 456 * 145 })
        {
            Fixture f; f.Off(); f.Tile(0, 0xAA, 0x55); f.display.Write(0xFF43, 3); f.On(); f.Dots(dots);
            { std::fstream file(path, std::ios::binary | std::ios::out); f.display.SaveState(&file); f.ppu.SaveState(&file); }
            f.Dots(70224); std::vector<uint32_t> expected(static_cast<uint32_t*>(f.display.GetVideoBuffer()), static_cast<uint32_t*>(f.display.GetVideoBuffer()) + 23040);
            auto ly = f.display.Read(0xFF44); auto dot = f.ppu.GetDot();
            { std::fstream file(path, std::ios::binary | std::ios::in); f.display.LoadState(&file); f.ppu.LoadState(&file); }
            f.Dots(70224); Assert::AreEqual(int(ly), int(f.display.Read(0xFF44))); Assert::AreEqual(int(dot), int(f.ppu.GetDot()));
            Assert::IsTrue(std::equal(expected.begin(), expected.end(), static_cast<uint32_t*>(f.display.GetVideoBuffer())));
        }
        std::filesystem::remove(path);
    }
    TEST_METHOD(HdmaTransfersOncePerHblankToSelectedBank)
    {
        NullSoundOutput sound; Emulator emulator(std::make_unique<VideoTests::Cartridge>(), &sound); emulator.SetFramePacingEnabled(false);
        emulator.WriteBus(0xC000, 42); emulator.WriteBus(0xC010, 84); emulator.WriteIO(0xFF4F, 1);
        emulator.WriteIO(0xFF51, 0xC0); emulator.WriteIO(0xFF52, 0); emulator.WriteIO(0xFF53, 0); emulator.WriteIO(0xFF54, 0); emulator.WriteIO(0xFF55, 0x81);
        emulator.Cycle(63); Assert::AreEqual(42, int(emulator.GetPpu()->ReadVideoRam(0x8000, 1)));
        Assert::AreEqual(0, int(emulator.GetPpu()->ReadVideoRam(0x8010, 1))); Assert::AreEqual(0, int(emulator.ReadIO(0xFF55)));
        emulator.Cycle(114); Assert::AreEqual(84, int(emulator.GetPpu()->ReadVideoRam(0x8010, 1))); Assert::AreEqual(255, int(emulator.ReadIO(0xFF55)));
    }
    TEST_METHOD(DoubleSpeedPreparationDoesNotChangePpuClock)
    {
        NullSoundOutput sound; Emulator emulator(std::make_unique<VideoTests::Cartridge>(), &sound); emulator.SetFramePacingEnabled(false);
        emulator.WriteIO(0xFF4D, 1); Assert::IsFalse(emulator.IsDoubleSpeedMode()); emulator.Cycle(10);
        Assert::AreEqual(40, int(emulator.GetPpu()->GetDot())); emulator.SetSpeedMode(); Assert::IsTrue(emulator.IsDoubleSpeedMode());
        emulator.Cycle(10); Assert::AreEqual(60, int(emulator.GetPpu()->GetDot()));
    }
    TEST_METHOD(LegacyStatesRejectedBeforeMutation)
    {
        NullSoundOutput sound; Emulator emulator(std::make_unique<VideoTests::Cartridge>(), &sound);
        auto path = std::filesystem::temp_directory_path() / "RetroGBm-old-video.state";
        SaveStateHeader header; header.version = 1;
        { std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char*>(&header), sizeof(header)); }
        emulator.GetCpu()->ProgramCounter = 123;
        Assert::ExpectException<std::runtime_error>([&] { emulator.LoadState(path.string()); });
        Assert::AreEqual(123, int(emulator.GetCpu()->ProgramCounter)); std::filesystem::remove(path);
    }
    TEST_METHOD(SaveStateRestoresSpeedAndDmgColourPreference)
    {
        Output output; NullSoundOutput sound; NullNetworkOutput network; Emulator emulator(&output, &sound, &network);
        emulator.SetFramePacingEnabled(false); emulator.LoadRom((RomRoot() / "cgb-acid2.gbc").string());
        emulator.SetDmgColourisation(false); emulator.WriteIO(0xFF4D, 1); emulator.SetSpeedMode(); emulator.Cycle(49);
        auto path = std::filesystem::temp_directory_path() / "RetroGBm-video-full.state";
        emulator.SaveState(path.string()); emulator.Cycle(36000);
        std::vector<uint32_t> expected(static_cast<uint32_t*>(emulator.GetVideoBuffer()), static_cast<uint32_t*>(emulator.GetVideoBuffer()) + 23040);
        auto dot = emulator.GetPpu()->GetDot(); auto ly = emulator.ReadIO(0xFF44);
        emulator.WriteIO(0xFF4D, 1); emulator.SetSpeedMode(); emulator.SetDmgColourisation(true);
        emulator.LoadState(path.string()); Assert::IsTrue(emulator.IsDoubleSpeedMode()); Assert::IsFalse(emulator.GetDisplay()->GetDmgColourisation());
        emulator.Cycle(36000); Assert::AreEqual(int(dot), int(emulator.GetPpu()->GetDot())); Assert::AreEqual(int(ly), int(emulator.ReadIO(0xFF44)));
        Assert::IsTrue(std::equal(expected.begin(), expected.end(), static_cast<uint32_t*>(emulator.GetVideoBuffer())));
        std::filesystem::remove(path);
    }
    TEST_METHOD(MooneyePpuAcceptance)
    {
        std::ostringstream failures;
        int tested = 0;
        for (const auto& entry : std::filesystem::directory_iterator(RomRoot() / "Gekkio" / "acceptance" / "ppu"))
        {
            if (entry.path().extension() != ".gb") continue;
            ++tested;
            Output output; NullSoundOutput sound; NullNetworkOutput network; Emulator emulator(&output, &sound, &network);
            emulator.SetFramePacingEnabled(false); emulator.LoadRom(entry.path().string());
            bool passed = false;
            for (int i = 0; i < 3000000; ++i)
            {
                emulator.Tick();
                auto cpu = emulator.GetCpu();
                if (cpu->GetRegister(RegisterType16::REG_BC) == 0x0305 && cpu->GetRegister(RegisterType16::REG_DE) == 0x080D && cpu->GetRegister(RegisterType16::REG_HL) == 0x1522) { passed = true; break; }
                if (emulator.GetOpCode() == 0x40 && cpu->GetRegister(RegisterType16::REG_BC) == 0x4242 && cpu->GetRegister(RegisterType16::REG_DE) == 0x4242) break;
            }
            std::string result = entry.path().filename().string() + (passed ? ": PASS\n" : ": FAIL\n");
            Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(result.c_str());
            if (!passed) failures << result;
            if (!passed)
            {
                for (int i = 0; i < 50000; ++i) emulator.Tick();
                auto path = std::filesystem::temp_directory_path() / (entry.path().stem().string() + ".ppm");
                std::ofstream file(path, std::ios::binary); file << "P6\n160 144\n255\n";
                auto pixels = static_cast<uint8_t*>(emulator.GetVideoBuffer());
                for (int i = 0; i < 23040; ++i) file.write(reinterpret_cast<char*>(pixels + i * 4), 3);
            }
        }
        const auto message = failures.str();
        Assert::AreEqual(12, tested, L"Bundled PPU ROMs are missing");
        Assert::IsTrue(message.empty(), std::wstring(message.begin(), message.end()).c_str());
    }
    TEST_METHOD(Acid2FrameCaptures)
    {
        for (const auto* name : { "dmg-acid2.gb", "cgb-acid2.gbc" })
        {
            Output output; NullSoundOutput sound; NullNetworkOutput network; Emulator emulator(&output, &sound, &network);
            emulator.SetFramePacingEnabled(false); Assert::IsTrue(emulator.LoadRom((RomRoot() / name).string()));
            for (int i = 0; i < 3000000 && output.frames < 60; ++i) emulator.Tick();
            Assert::IsTrue(output.frames >= 60);
            auto path = std::filesystem::temp_directory_path() / (std::string(name) + ".ppm");
            std::ofstream file(path, std::ios::binary); file << "P6\n160 144\n255\n";
            auto pixels = static_cast<uint8_t*>(emulator.GetVideoBuffer());
            for (int i = 0; i < 23040; ++i) file.write(reinterpret_cast<char*>(pixels + i * 4), 3);
        }
    }
};
}
