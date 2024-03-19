#include "CppUnitTest.h"

#include <RetroGBm/Cpu.h>
#include <RetroGBm/Cartridge.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/PixelPipeline.h>
#include <RetroGBm/PixelProcessor.h>

#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	PixelProcessor* m_Ppu = nullptr;

	static PixelPipeline BuildPixelPipeline()
	{
		Cartridge cartidge;
		Cpu _cpu(&cartidge);

		Display* display = new Display();
		display->GetContext()->ly = 0;
		display->GetContext()->scx = 0;
		display->GetContext()->scy = 0;

		m_Ppu = new PixelProcessor(display, &_cpu);
		return PixelPipeline(m_Ppu, display, &cartidge);
	}

	static uint8_t ConvertAttribute(const BackgroundWindowAttribute& attribute)
	{
		uint8_t result = 0;
		result |= (attribute.colour_palette & 0b111);
		result |= (attribute.bank << 3);
		result |= (attribute.flip_x << 5);
		result |= (attribute.flip_y << 6);
		result |= (attribute.priority << 7);

		return result;
	}

	TEST_CLASS(PixelPipelineTests)
	{
	public:
		TEST_METHOD(Fetch_GetTileAttributesFromBank1_TileBankIs1AndPaletteIs5)
		{
			// Arrange
			PixelPipeline pipeline = BuildPixelPipeline();

			BackgroundWindowAttribute attribute = {};
			attribute.bank = 1;
			attribute.colour_palette = 5;

			m_Ppu->SetVideoRamBank(1);
			m_Ppu->WriteVideoRam(0x9800, ConvertAttribute(attribute));
			m_Ppu->SetVideoRamBank(0);

			const_cast<PixelProcessorContext*>(m_Ppu->GetContext())->dots = 1;

			// Act
			pipeline.PipelineProcess();

			// Assert
			Assert::AreEqual(5, static_cast<int>(pipeline.GetContext()->background_window_attribute.colour_palette));
			Assert::AreEqual(1, static_cast<int>(pipeline.GetContext()->background_window_attribute.bank));
		}
	};
}