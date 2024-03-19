#include "RetroGBm/Pch.h"
#include "RetroGBm/HighTimer.h"
#include <ctime>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#endif

HighTimer::HighTimer()
{
#ifdef _WIN32
	uint64_t counts_per_second = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);
#else
	uint64_t counts_per_second = std::chrono::high_resolution_clock::duration::period::den / std::chrono::high_resolution_clock::duration::period::num;
#endif
	
	m_SecondsPerCount = 1.0 / static_cast<double>(counts_per_second);

	Reset();
}

void HighTimer::Start()
{
#ifdef _WIN32
	__int64 start_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
#else
	auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
	uint64_t start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
#endif

	if (m_Stopped)
	{
		m_PausedTime += (start_time - m_StopTime);

		m_PreviousTime = start_time;
		m_StopTime = 0;
		m_Stopped = false;
	}
}

void HighTimer::Stop()
{
	if (!m_Stopped)
	{
#ifdef _WIN32
		uint64_t current_time = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
#else
		auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
		uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
#endif

		m_StopTime = current_time;
		m_Stopped = true;
	}
}

void HighTimer::Reset()
{
#ifdef _WIN32
	uint64_t current_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
#else
	auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
	uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
#endif

	m_BaseTime = current_time;
	m_PreviousTime = current_time;
	m_StopTime = 0;
	m_Stopped = false;

	Tick();
}

void HighTimer::Tick()
{
	if (m_Stopped)
	{
		m_DeltaTime = 0.0;
		return;
	}

#ifdef _WIN32
	uint64_t current_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
#else
	auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
	uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
#endif

	m_CurrentTime = current_time;

	// Time difference between this frame and the previous.
	m_DeltaTime = (m_CurrentTime - m_PreviousTime) * m_SecondsPerCount;

	// Prepare for next frame.
	m_PreviousTime = m_CurrentTime;

	if (m_DeltaTime < 0.0)
	{
		m_DeltaTime = 0.0;
	}
}

double HighTimer::DeltaTime()
{
	return static_cast<double>(m_DeltaTime);
}

double HighTimer::TotalTime()
{
	if (m_Stopped)
	{
		return static_cast<double>(((m_StopTime - m_PausedTime) - m_BaseTime) * m_SecondsPerCount);
	}
	else
	{
		return static_cast<double>((m_CurrentTime - m_BaseTime) * m_SecondsPerCount);
	}
}
