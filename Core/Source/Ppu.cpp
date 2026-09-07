#include "RetroGBm/Pch.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/Cpu.h"
#include "RetroGBm/VideoState.h"
#include <cmath>

Ppu::Ppu(IBus*, Cpu* cpu, Display* display, BaseCartridge*) : m_Display(display), m_Cpu(cpu)
{
	if (m_Display)
	{
		m_Display->AttachPpu(this);
	}

	Init();
}

void Ppu::ResetClock()
{
	m_Deadline = m_FpsStart = std::chrono::steady_clock::now();
	m_Fps = m_FrameCount = 0;
}

void Ppu::Init()
{
	m_Vram.fill(0);
	m_Oam.fill(0);
	m_Bank = 0;
	m_Dot = 0;
	m_Line = 0;
	m_WindowLine = 0;
	m_WindowTriggered = 0;
	m_StatLine = 0;
	m_FirstLine = 0;
	m_HBlank = false;
	m_Frame = false;
	m_BackgroundFifo.clear();
	m_ObjectFifo = {};
	m_Objects = {};
	m_Fetched = {};
	m_Count = 0;
	m_WindowActive = 0;
	m_WindowUsed = 0;
	m_ObjectStall = 0;
	m_TransferDots = 0;
	m_TileAddress = 0;
	m_FetchPhase = 0;
	m_FetchX = 0;
	m_Tile = 0;
	m_Attribute = 0;
	m_Low = 0;
	m_High = 0;
	m_X = 0;
	m_Discard = 0;
	m_Startup = 0;
	m_CurrentObject = 0;

	if (m_Display)
	{
		SetMode(m_Display->IsLcdEnabled() ? LcdMode::OAM : LcdMode::HBlank);
		BeginLine();
	}

	ResetClock();
}

void Ppu::LcdEnableChanged()
{
	m_Dot = 0;
	m_Line = 0;
	m_WindowLine = 0;
	m_WindowTriggered = 0;
	m_FirstLine = m_Display->IsLcdEnabled();
	m_BackgroundFifo.clear();
	m_ObjectFifo = {};
	m_ObjectStall = 0;
	m_HBlank = false;
	m_Frame = false;
	m_Display->SetStatus(LcdMode::HBlank, 0, m_Display->IsLcdEnabled());
	if (m_Display->IsLcdEnabled())
	{
		UpdateStat();
	}
	else
	{
		m_StatLine = (m_Display->Read(0xFF41) & 0x44) == 0x44;
	}

	BeginLine();
	if (!m_Display->IsLcdEnabled())
	{
		m_Display->Blank();
	}

	ResetClock();
}

void Ppu::UpdateStat()
{
	if (!m_Display || !m_Display->IsLcdEnabled())
	{
		return;
	}

	auto mode = m_Display->GetLcdMode();
	uint8_t ly = m_Line == 153 && m_Dot >= 4 ? 0 : uint8_t(m_Line + (m_Dot >= 452 && m_Line < 153));
	m_Display->SetStatus(mode, ly, true, m_Dot >= 452 && m_Line < 153);
	uint8_t stat = m_Display->Read(0xFF41);
	bool signal = m_Display->IsLcdEnabled() &&
		(((stat & 0x40) && (stat & 4)) ||
			(mode == LcdMode::HBlank && (stat & 8)) ||
			(mode == LcdMode::VBlank && (stat & 0x10)) ||
			(mode == LcdMode::OAM && (stat & 0x20)) ||
			(!m_Display->IsColour() && m_Line == 144 && m_Dot < 4 && (stat & 0x20)));
	if (signal && !m_StatLine && m_Cpu)
	{
		m_Cpu->RequestInterrupt(InterruptFlag::STAT);
	}

	m_StatLine = signal;
}

void Ppu::SetMode(LcdMode mode)
{
	m_Display->SetStatus(mode, m_Line);
	UpdateStat();
}

void Ppu::BeginLine()
{
	m_Count = 0;
	m_Fetched.fill(0);
	m_WindowUsed = 0;
	m_WindowActive = 0;

	if (m_Line == m_Display->Read(0xFF4A))
	{
		m_WindowTriggered = 1;
	}
}

uint8_t Ppu::ReadVideoRam(uint16_t address, uint8_t bank) const
{
	return m_Vram[(bank & 1) * 0x2000 + (address & 0x1FFF)];
}

uint8_t Ppu::ReadVideoRam(uint16_t address) const
{
	if (m_Display && m_Display->IsLcdEnabled() && (m_Display->GetLcdMode() == LcdMode::PixelTransfer ||
		(!m_FirstLine && m_Line < 144 && m_Dot >= 76 && m_Dot < 80)))
	{
		return 0xFF;
	}

	return ReadVideoRam(address, m_Bank);
}

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	if (m_Display && m_Display->IsLcdEnabled() && m_Display->GetLcdMode() == LcdMode::PixelTransfer)
	{
		return;
	}

	WriteVideoRamDma(address, value);
}

void Ppu::WriteVideoRamDma(uint16_t address, uint8_t value)
{
	m_Vram[m_Bank * 0x2000 + (address & 0x1FFF)] = value;
}

uint8_t Ppu::ReadOam(uint16_t address) const
{
	if (m_Display && m_Display->IsLcdEnabled() && ((uint8_t(m_Display->GetLcdMode()) & 2) || (m_Line < 144 && m_Dot >= 452)))
	{
		return 0xFF;
	}

	auto offset = address >= 0xFE00 ? address - 0xFE00 : address;
	return offset < 160 ? m_Oam[offset] : 0xFF;
}

void Ppu::WriteOam(uint16_t address, uint8_t value)
{
	// OAM reads are blocked before the scan; writes remain possible until mode 2.
	if (m_Display && m_Display->IsLcdEnabled() && (m_Display->GetLcdMode() == LcdMode::PixelTransfer ||
		(m_Display->GetLcdMode() == LcdMode::OAM && m_Dot < 76)))
	{
		return;
	}

	WriteOamDma(address, value);
}

void Ppu::WriteOamDma(uint16_t address, uint8_t value)
{
	auto offset = address >= 0xFE00 ? address - 0xFE00 : address;
	if (offset < 160)
	{
		m_Oam[offset] = value;
	}
}

uint8_t Ppu::GetVideoRamBank() const
{
	return m_Display && m_Display->IsColour() ? uint8_t(0xFE | m_Bank) : 0xFF;
}

void Ppu::SetVideoRamBank(uint8_t value)
{
	m_Bank = m_Display && m_Display->IsColour() ? value & 1 : 0;
}

void Ppu::Tick()
{
	if (!m_Display || !m_Display->IsLcdEnabled())
	{
		return;
	}

	if (m_Line < 144)
	{
		if (m_Dot < (m_FirstLine ? 78 : 80))
		{
			if ((m_Dot & 1) && m_Count < 10)
			{
				int i = m_Dot / 2, y = int(m_Oam[i * 4]) - 16;
				int height = (m_Display->Read(0xFF40) & 4) ? 16 : 8;
				if (m_Line >= y && m_Line < y + height)
				{
					m_Objects[m_Count++] = { m_Oam[i * 4], m_Oam[i * 4 + 1], m_Oam[i * 4 + 2], m_Oam[i * 4 + 3], uint8_t(i) };
				}
			}
		}
		else if (m_Display->GetLcdMode() == LcdMode::PixelTransfer)
		{
			Transfer();
		}
	}
	++m_Dot;
	if (m_Line < 144 && m_Dot == (m_FirstLine ? 78 : 80))
	{
		BeginTransfer();
	}
	if (m_Dot == 452 || ((m_Line == 153 || m_Line == 144) && m_Dot == 4))
	{
		UpdateStat();
	}

	if (m_Dot == 456)
	{
		m_Dot = 0;
		m_FirstLine = 0;
		if (m_Line < 144 && m_WindowUsed)
		{
			++m_WindowLine;
		}

		++m_Line;
		if (m_Line == 144)
		{
			SetMode(LcdMode::VBlank);
			if (m_Cpu)
			{
				m_Cpu->RequestInterrupt(InterruptFlag::VBlank);
			}

			m_Display->UpdateDisplay();
			m_Frame = true;
			++m_FrameCount;
			auto now = std::chrono::steady_clock::now();
			double elapsed = std::chrono::duration<double>(now - m_FpsStart).count();
			if (elapsed >= 1)
			{
				m_Fps = int(m_FrameCount / elapsed);
				m_FrameCount = 0;
				m_FpsStart = now;
			}
		}
		else if (m_Line == 154)
		{
			m_Line = 0;
			m_WindowLine = 0;
			m_WindowTriggered = 0;
			BeginLine();
			SetMode(LcdMode::OAM);
		}
		else if (m_Line < 144)
		{
			BeginLine();
			SetMode(LcdMode::OAM);
		}
		else
		{
			UpdateStat();
		}
	}
}

void Ppu::BeginTransfer()
{
	m_X = 0;
	m_TransferDots = 0;
	m_Discard = m_Display->Read(0xFF43) & 7;
	m_Startup = 12;
	m_FetchPhase = 0;
	m_FetchX = 0;
	m_ObjectStall = 0;
	m_BackgroundFifo.clear();
	m_ObjectFifo = {};
	SetMode(LcdMode::PixelTransfer);
	m_LastObjectTile = -1;
}

void Ppu::Fetch()
{
	uint8_t lcdc = m_Display->Read(0xFF40);
	if (m_FetchPhase == 0)
	{
		uint8_t y = m_WindowActive ? m_WindowLine : uint8_t(m_Line + m_Display->Read(0xFF42));
		uint8_t x = m_WindowActive ? m_FetchX : uint8_t((m_Display->Read(0xFF43) >> 3) + m_FetchX);
		uint16_t map = (lcdc & (m_WindowActive ? 0x40 : 8)) ? 0x9C00 : 0x9800;
		uint16_t address = map + (y >> 3) * 32 + (x & 31);
		m_Tile = ReadVideoRam(address, 0);
		m_Attribute = m_Display->IsColour() ? ReadVideoRam(address, 1) : 0;
		uint8_t row = y & 7;
		if (m_Attribute & 0x40)
		{
			row = 7 - row;
		}

		m_TileAddress = uint16_t(
			(lcdc & 0x10 ? 0x8000 + m_Tile * 16 : 0x9000 + int8_t(m_Tile) * 16) + row * 2);
	}
	if (m_FetchPhase == 2)
	{
		m_Low = ReadVideoRam(m_TileAddress, (m_Attribute >> 3) & 1);
	}
	if (m_FetchPhase == 4)
	{
		m_High = ReadVideoRam(m_TileAddress + 1, (m_Attribute >> 3) & 1);
	}
	if (m_FetchPhase == 5)
	{
		if (m_BackgroundFifo.size() > 8)
		{
			return;
		}

		for (int i = 0; i < 8; ++i)
		{
			int bit = (m_Attribute & 0x20) ? i : 7 - i;
			m_BackgroundFifo.push_back(
				{ uint8_t(((m_Low >> bit) & 1) | (((m_High >> bit) & 1) << 1)),
				  uint8_t(m_Attribute & 7),
				  uint8_t(m_Attribute >> 7) });
		}
		++m_FetchX;
	}

	m_FetchPhase = (m_FetchPhase + 1) & 7;
}

void Ppu::MergeObject()
{
	const auto& obj = m_Objects[m_CurrentObject];
	int height = (m_Display->Read(0xFF40) & 4) ? 16 : 8;
	int row = m_Line + 16 - obj.y;
	if (obj.flags & 0x40)
	{
		row = height - 1 - row;
	}

	uint8_t tile = height == 16 ? obj.tile & 0xFE : obj.tile;
	uint16_t address = uint16_t(0x8000 + tile * 16 + row * 2);
	uint8_t bank = m_Display->IsColour() ? (obj.flags >> 3) & 1 : 0;
	uint8_t low = ReadVideoRam(address, bank);
	uint8_t high = ReadVideoRam(address + 1, bank);
	for (int i = 0; i < 8; ++i)
	{
		int dest = int(obj.x) - 8 + i - m_X;
		if (dest < 0 || dest >= 8)
		{
			continue;
		}

		int bit = obj.flags & 0x20 ? i : 7 - i;
		uint8_t colour = uint8_t(((low >> bit) & 1) | (((high >> bit) & 1) << 1));
		auto& pixel = m_ObjectFifo[dest];
		bool ahead = m_Display->CoordinatePriority()
			? (obj.x < pixel.objectX || (obj.x == pixel.objectX && obj.order < pixel.order))
			: obj.order < pixel.order;
		if (colour && (!pixel.colour || ahead))
		{
			pixel = {
				colour,
				uint8_t(m_Display->IsColour() ? obj.flags & 7 : (obj.flags >> 4) & 1),
				uint8_t(obj.flags >> 7),
				obj.order,
				obj.x
			};
		}
	}
}

void Ppu::Transfer()
{
	++m_TransferDots;
	if (m_ObjectStall)
	{
		if (--m_ObjectStall == 0)
		{
			MergeObject();
		}

		return;
	}

	if (m_Startup)
	{
		Fetch();
		--m_Startup;
		return;
	}

	uint8_t lcdc = m_Display->Read(0xFF40);
	int wx = int(m_Display->Read(0xFF4B)) - 7;
	bool windowEnabled = (lcdc & 0x20) && (m_Display->IsColour() || (lcdc & 1));

	if (!m_WindowActive && m_WindowTriggered && windowEnabled && wx < 160 && m_X >= wx && !m_Discard)
	{
		m_WindowActive = 1;
		m_WindowUsed = 1;
		m_FetchX = 0;
		m_FetchPhase = 0;
		m_BackgroundFifo.clear();
		m_Discard = uint8_t(std::max(0, -wx));
	}

	if (!m_Discard && ((lcdc & 2) || m_Display->IsColour()))
	{
		int selected = -1;
		for (int i = 0; i < m_Count; ++i)
		{
			if (!m_Fetched[i] && m_Objects[i].x < 168 && int(m_Objects[i].x) - 8 <= m_X &&
				(selected < 0 || m_Objects[i].x < m_Objects[selected].x))
			{
				selected = i;
			}
		}

		if (selected >= 0)
		{
			m_Fetched[selected] = 1;
			m_CurrentObject = uint8_t(selected);
			int position = m_Objects[selected].x + m_Display->Read(0xFF43);
			int tile = position / 8;
			int alignmentDelay = tile == m_LastObjectTile ? 0 : std::max(0, 5 - (position & 7));
			// The first object fetch overlaps the initial background pipeline by three dots.
			int overlap = m_LastObjectTile < 0 ? 3 : 0;
			m_LastObjectTile = int16_t(tile);
			m_ObjectStall = uint8_t(5 + alignmentDelay - overlap);
			return;
		}
	}

	if (!m_BackgroundFifo.empty())
	{
		auto bg = m_BackgroundFifo.front();
		m_BackgroundFifo.pop_front();
		if (m_Discard)
		{
			--m_Discard;
		}
		else
		{
			if (!m_Display->IsColour() && !(lcdc & 1))
			{
				bg.colour = 0;
			}

			auto obj = m_ObjectFifo[0];
			bool backgroundWins = bg.colour && (obj.priority || (m_Display->IsColour() && bg.priority));
			if (m_Display->IsColour() && !(lcdc & 1))
			{
				backgroundWins = false;
			}

			uint32_t colour = (lcdc & 2) && obj.colour && !backgroundWins
				? m_Display->GetColourFromObjectPalette(obj.palette, obj.colour)
				: m_Display->GetColourFromBackgroundPalette(bg.palette, bg.colour);
			m_Display->SetVideoBufferPixel(m_X++, m_Line, colour);
			for (int i = 0; i < 7; ++i)
			{
				m_ObjectFifo[i] = m_ObjectFifo[i + 1];
			}

			m_ObjectFifo[7] = {};
			if (m_X == 160)
			{
				SetMode(LcdMode::HBlank);
				m_HBlank = true;
				return;
			}
		}
	}

	Fetch();
}

void Ppu::SetSpeedMultipler(float value)
{
	if (std::isfinite(value) && value > 0)
	{
		m_Speed = value;
		ResetClock();
	}
}

void Ppu::PaceFrame()
{
	auto period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(70224.0 / 4194304.0 / m_Speed));
	m_Deadline += period;
	auto now = std::chrono::steady_clock::now();
	if (m_Deadline > now)
	{
		std::this_thread::sleep_until(m_Deadline);
	}
	else if (now - m_Deadline > period * 4)
	{
		m_Deadline = now;
	}
}

void Ppu::SaveState(std::fstream* file)
{
	using VideoState::Write;
	for (auto value : m_Vram)
	{
		Write(*file, value);
	}

	for (auto value : m_Oam)
	{
		Write(*file, value);
	}

	for (auto value : m_Fetched)
	{
		Write(*file, value);
	}

	for (auto o : m_Objects)
	{
		Write(*file, o.y);
		Write(*file, o.x);
		Write(*file, o.tile);
		Write(*file, o.flags);
		Write(*file, o.order);
	}

	auto pixel = [&](VideoPixel p)
	{
		Write(*file, p.colour);
		Write(*file, p.palette);
		Write(*file, p.priority);
		Write(*file, p.order);
		Write(*file, p.objectX);
	};

	Write(*file, uint8_t(m_BackgroundFifo.size()));
	for (auto p : m_BackgroundFifo)
	{
		pixel(p);
	}

	for (auto p : m_ObjectFifo)
	{
		pixel(p);
	}

	Write(*file, m_Dot);
	Write(*file, m_TransferDots);
	Write(*file, m_Line);
	Write(*file, m_FirstLine);
	Write(*file, m_Bank);
	Write(*file, m_Count);
	Write(*file, m_WindowLine);
	Write(*file, m_WindowTriggered);
	Write(*file, m_WindowActive);
	Write(*file, m_WindowUsed);
	Write(*file, m_StatLine);
	Write(*file, m_FetchPhase);
	Write(*file, m_FetchX);
	Write(*file, m_Tile);
	Write(*file, m_Attribute);
	Write(*file, m_Low);
	Write(*file, m_High);
	Write(*file, m_TileAddress);
	Write(*file, m_X);
	Write(*file, m_LastObjectTile);
	Write(*file, m_Discard);
	Write(*file, m_Startup);
	Write(*file, m_ObjectStall);
	Write(*file, m_CurrentObject);
	Write(*file, m_HBlank);
	Write(*file, m_Frame);
}

void Ppu::LoadState(std::fstream* file)
{
	using VideoState::Read;
	for (auto& value : m_Vram)
	{
		Read(*file, value);
	}

	for (auto& value : m_Oam)
	{
		Read(*file, value);
	}

	for (auto& value : m_Fetched)
	{
		Read(*file, value);
	}

	for (auto& o : m_Objects)
	{
		Read(*file, o.y);
		Read(*file, o.x);
		Read(*file, o.tile);
		Read(*file, o.flags);
		Read(*file, o.order);
	}

	auto pixel = [&](VideoPixel& p)
	{
		Read(*file, p.colour);
		Read(*file, p.palette);
		Read(*file, p.priority);
		Read(*file, p.order);
		Read(*file, p.objectX);
	};

	uint8_t size;
	Read(*file, size);
	if (size > 16)
	{
		throw std::runtime_error("Invalid video FIFO state");
	}

	m_BackgroundFifo.resize(size);
	for (auto& p : m_BackgroundFifo)
	{
		pixel(p);
	}

	for (auto& p : m_ObjectFifo)
	{
		pixel(p);
	}

	Read(*file, m_Dot);
	Read(*file, m_TransferDots);
	Read(*file, m_Line);
	Read(*file, m_FirstLine);
	Read(*file, m_Bank);
	Read(*file, m_Count);
	Read(*file, m_WindowLine);
	Read(*file, m_WindowTriggered);
	Read(*file, m_WindowActive);
	Read(*file, m_WindowUsed);
	Read(*file, m_StatLine);
	Read(*file, m_FetchPhase);
	Read(*file, m_FetchX);
	Read(*file, m_Tile);
	Read(*file, m_Attribute);
	Read(*file, m_Low);
	Read(*file, m_High);
	Read(*file, m_TileAddress);
	Read(*file, m_X);
	Read(*file, m_LastObjectTile);
	Read(*file, m_Discard);
	Read(*file, m_Startup);
	Read(*file, m_ObjectStall);
	Read(*file, m_CurrentObject);
	Read(*file, m_HBlank);
	Read(*file, m_Frame);
	if (m_Dot >= 456 || m_Line >= 154 || m_Bank > 1 || m_Count > 10 || m_CurrentObject >= 10 || m_FetchPhase > 7 || m_X < 0 || m_X > 160)
	{
		throw std::runtime_error("Invalid PPU state");
	}

	ResetClock();
}
