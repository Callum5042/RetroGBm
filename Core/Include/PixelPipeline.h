//#pragma once
//
//#include <queue>
//
//class PixelProcessor;
//class Display;
//
//enum class FetchState
//{
//	GetTile,
//	ReadData1,
//	ReadData2,
//	Idle
//};
//
//enum class FetchTileByte
//{
//	ByteLow,
//	ByteHigh
//};
//
//enum FetchTileType
//{
//	Background,
//	Window,
//	Object
//};
//
//struct PixelData
//{
//	int colour_index;
//	int palette;
//	int sprite_priority;
//	int background_priority;
//};
//
//struct BackgroundWindowAttribute
//{
//	uint8_t colour_palette = 0;
//	uint8_t bank = 0;
//	bool flip_x = false;
//	bool flip_y = false;
//	bool priority = false;
//};
//
//struct PixelPipelineContext
//{
//	int dots = 0;
//	FetchState fetch_state = FetchState::GetTile;
//	FetchTileType tile_type = FetchTileType::Background;
//
//	std::deque<PixelData> background_queue;
//	// std::queue<PixelData> m_ObjectQueue;
//
//	// Fetch data
//	int fetcher_x = 0;
//	int fetcher_y = 0;
//
//	int tile_id = 0;
//	BackgroundWindowAttribute background_window_attribute;
//
//	uint8_t data_low = 0;
//	uint8_t data_high = 0;
//};
//
//class PixelPipeline
//{
//	PixelProcessor* m_Ppu = nullptr;
//	Display* m_Display = nullptr;
//
//public:
//	PixelPipeline(PixelProcessor* ppu, Display* display);
//	virtual ~PixelPipeline() = default;
//
//	bool Fetch(PixelData* pixel_data);
//
//	inline const PixelPipelineContext* GetContext() { return &m_Context; }
//
//private:
//	PixelPipelineContext m_Context;
//
//	// Fetch state functions
//	void ReadTile();
//	void ReadData1();
//	void ReadData2();
//	void Idle();
//
//	uint8_t ReadByte(FetchTileByte tile_byte);
//	void AddPixels();
//};