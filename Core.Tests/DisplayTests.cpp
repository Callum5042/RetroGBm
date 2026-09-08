#include "CppUnitTest.h"
#include "VideoTestFixture.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace VideoTests;
namespace CoreTests
{
TEST_CLASS(DisplayTests)
{
public:
    TEST_METHOD(DrMarioUsesCgbCompatibilityPalettesByDefault)
    {
        VideoTests::Cartridge cartridge(false); cartridge.SetHeader("DR.MARIO");
        Display display(&cartridge, nullptr); display.Write(0xFF47, 0xE4); display.Write(0xFF48, 0xE4); display.Write(0xFF49, 0xE4);
        Assert::IsTrue(display.GetDmgColourisation()); Assert::IsFalse(display.IsColour());
        Assert::AreEqual(0xFFFF0000u, display.GetColourFromBackgroundPalette(0, 2));
        Assert::AreEqual(0xFFFF0000u, display.GetColourFromObjectPalette(0, 2));
        Assert::AreEqual(0xFF8484FFu, display.GetColourFromObjectPalette(1, 1));
        display.SetDmgColourisation(false);
        Assert::AreEqual(0xFF555555u, display.GetColourFromBackgroundPalette(0, 2));
        display.SetDmgColourisation(true); display.Write(0xFF47, 0x1B);
        Assert::AreEqual(0xFFFF0000u, display.GetColourFromBackgroundPalette(0, 1));
    }
    TEST_METHOD(CompatibilityPaletteChecksLicenseAndDuplicateTitle)
    {
        VideoTests::Cartridge cartridge(false); cartridge.SetHeader("DR.MARIO", 0x33);
        Display licensed(&cartridge, nullptr); licensed.Write(0xFF47, 0xE4);
        Assert::AreEqual(0xFFFF0000u, licensed.GetColourFromBackgroundPalette(0, 2));
        cartridge.SetHeader("DR.MARIO", 2); Display unlicensed(&cartridge, nullptr); unlicensed.Write(0xFF47, 0xE4);
        Assert::AreNotEqual(0xFFFF0000u, unlicensed.GetColourFromBackgroundPalette(0, 2));
        std::string title(16, '\0'); title[3] = 'B'; title[15] = char(0xB3 - 'B'); cartridge.SetHeader(title);
        Display first(&cartridge, nullptr); first.Write(0xFF47, 0xE4);
        title[3] = 'U'; title[15] = char(0xB3 - 'U'); cartridge.SetHeader(title);
        Display second(&cartridge, nullptr); second.Write(0xFF47, 0xE4);
        Assert::AreNotEqual(first.GetColourFromBackgroundPalette(0, 1), second.GetColourFromBackgroundPalette(0, 1));
    }
    TEST_METHOD(DmgColourSettingDoesNotChangeNativeCgbPalettes)
    {
        Fixture f; f.Off(); f.Palette(false, 2, 1, 31);
        f.display.SetDmgColourisation(true); auto colour = f.display.GetColourFromBackgroundPalette(2, 1);
        f.display.SetDmgColourisation(false); Assert::AreEqual(colour, f.display.GetColourFromBackgroundPalette(2, 1));
    }
    TEST_METHOD(PalettePortsWrapAndPreserveBytes)
    {
        Fixture f; f.Off(); f.display.Write(0xFF68, 0xBF); f.display.Write(0xFF69, 0x12);
        Assert::AreEqual(0xC0, int(f.display.Read(0xFF68)));
        f.display.Write(0xFF68, 63); Assert::AreEqual(0x12, int(f.display.Read(0xFF69)));
        f.display.Write(0xFF6A, 0xBF); f.display.Write(0xFF6B, 0x34);
        Assert::AreEqual(0xC0, int(f.display.Read(0xFF6A)));
    }
    TEST_METHOD(Rgb555UsesRgbaBytes)
    {
        Fixture f; f.Off(); f.Palette(false, 3, 1, 31); f.Palette(false, 3, 2, 31 << 5); f.Palette(true, 7, 3, 31 << 10);
        Assert::AreEqual(0xFF0000FFu, f.display.GetColourFromBackgroundPalette(3, 1));
        Assert::AreEqual(0xFF00FF00u, f.display.GetColourFromBackgroundPalette(3, 2));
        Assert::AreEqual(0xFFFF0000u, f.display.GetColourFromObjectPalette(7, 3));
    }
    TEST_METHOD(Mode3PaletteWritesBlockedButIncrement)
    {
        Fixture f; f.display.Write(0xFF68, 0x80); f.Dots(80); f.display.Write(0xFF69, 0);
        Assert::AreEqual(0xC1, int(f.display.Read(0xFF68))); Assert::AreEqual(255, int(f.display.Read(0xFF69)));
        f.Off(); f.display.Write(0xFF68, 0); Assert::AreEqual(255, int(f.display.Read(0xFF69)));
    }
    TEST_METHOD(DmgPalettesAndCgbRegisterGating)
    {
        Fixture f(false); f.display.Write(0xFF47, 0xE4);
        Assert::AreEqual(0xFFAAAAAAu, f.display.GetColourFromBackgroundPalette(0, 1));
        f.display.Write(0xFF68, 0); Assert::AreEqual(255, int(f.display.Read(0xFF68)));
        f.ppu.SetVideoRamBank(1); Assert::AreEqual(255, int(f.ppu.GetVideoRamBank()));
    }
    TEST_METHOD(ReadOnlyStatusAndLcdBlanking)
    {
        Fixture f; f.display.Write(0xFF41, 255); f.display.Write(0xFF44, 123);
        Assert::AreEqual(0, int(f.display.Read(0xFF44))); Assert::AreEqual(0xFE, int(f.display.Read(0xFF41)));
        void* buffer = f.display.GetVideoBuffer(); f.Off(); f.Dots(1000);
        Assert::AreEqual(0, int(f.display.Read(0xFF44))); Assert::AreEqual(0xFFFFFFFFu, f.Pixel(0));
        Assert::IsTrue(buffer == f.display.GetVideoBuffer()); Assert::AreEqual(640, f.output.pitch);
    }
};
}
