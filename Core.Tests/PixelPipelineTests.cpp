#include "CppUnitTest.h"

#include <Cpu.h>
#include <Cartridge.h>
#include <Display.h>
#include <PixelPipeline.h>
#include <PixelProcessor.h>

#include <cstdint>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	static PixelPipeline BuildPixelPipeline()
	{
		Cartridge cartidge;
		Cpu _cpu(&cartidge);
		Display* display = new Display();
		display->GetContext()->ly = 0;
		display->GetContext()->scx = 0;
		display->GetContext()->scy = 0;

		PixelProcessor* ppu = new PixelProcessor(display, &_cpu);
		return PixelPipeline(ppu, display);
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
		TEST_METHOD(Fetch_PipelineIsEmpty_ReturnsFalse)
		{
			// Arrange
			PixelPipeline pipeline = BuildPixelPipeline();

			// Act
			bool result = pipeline.Fetch(nullptr);

			// Assert
			Assert::IsFalse(result);
		}

		TEST_METHOD(Fetch_FetchStateIsGetTileAndFetchedCalledTwice_OnlyAdvanceFetchStateOnceButPush2Pixels)
		{
			// Arrange
			PixelPipeline pipeline = BuildPixelPipeline();
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->dots = 0;

			// Act
			PixelData pixel_data;
			pipeline.Fetch(&pixel_data);
			pipeline.Fetch(&pixel_data);

			// Assert
			Assert::IsTrue(pipeline.GetContext()->fetch_state == FetchState::ReadData1);
		}

		TEST_METHOD(Fetch_StateIsGetTile_FetchTileIdAndAttributeData)
		{
			// Arrange
			Cartridge cartidge;
			Cpu _cpu(&cartidge);
			Display display;

			PixelProcessor ppu(&display);
			ppu.WriteVideoRam(0x9800, 0x12);

			BackgroundWindowAttribute attribute = {};
			attribute.colour_palette = 7;
			attribute.bank = 1;

			ppu.SetVideoRamBank(1);
			ppu.WriteVideoRam(0x9800, ConvertAttribute(attribute));
			ppu.SetVideoRamBank(0);

			PixelPipeline pipeline(&ppu, &display);
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->dots = 0; 
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->fetch_state = FetchState::GetTile;

			// Act
			PixelData pixel_data;
			pipeline.Fetch(&pixel_data);

			// Assert
			Assert::AreEqual(0x12, pipeline.GetContext()->tile_id);
			Assert::AreEqual(attribute.colour_palette, pipeline.GetContext()->background_window_attribute.colour_palette);
			Assert::AreEqual(attribute.bank, pipeline.GetContext()->background_window_attribute.bank);
		}

		TEST_METHOD(Fetch_StateIsReadData1_GetLowByte)
		{
			// Arrange
			Cartridge cartidge;
			Cpu _cpu(&cartidge);
			Display display;
			display.GetContext()->lcdc = (1 << 4);

			PixelProcessor ppu(&display);
			ppu.WriteVideoRam(0x8000, 0x80);

			PixelPipeline pipeline(&ppu, &display);
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->dots = 0; 
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->fetch_state = FetchState::ReadData1;

			// Act
			PixelData pixel_data;
			pipeline.Fetch(&pixel_data);

			// Assert
			Assert::AreEqual(0x80, static_cast<int>(pipeline.GetContext()->data_low));
		}

		TEST_METHOD(Fetch_StateIsReadData2_GetHighByte)
		{
			// Arrange
			Cartridge cartidge;
			Cpu _cpu(&cartidge);
			Display display;
			display.GetContext()->lcdc = (1 << 4);

			PixelProcessor ppu(&display);
			ppu.WriteVideoRam(0x8001, 0x80);

			PixelPipeline pipeline(&ppu, &display);
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->dots = 0; 
			const_cast<PixelPipelineContext*>(pipeline.GetContext())->fetch_state = FetchState::ReadData2;

			// Act
			PixelData pixel_data;
			pipeline.Fetch(&pixel_data);

			// Assert
			Assert::AreEqual(0x80, static_cast<int>(pipeline.GetContext()->data_high));
		}
	};
}