#include "Pch.h"
#include "HighTimer.h"
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

HighTimer::HighTimer()
{
	__int64 counts_per_second = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);


	m_SecondsPerCount = 1.0 / static_cast<double>(counts_per_second);

	Reset();
}

void HighTimer::Start()
{
	__int64 start_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);

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
		__int64 current_time = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

		m_StopTime = current_time;
		m_Stopped = true;
	}
}

void HighTimer::Reset()
{
	__int64 current_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

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

	__int64 current_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

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
		return static_cast<double>(((m_CurrentTime - m_PausedTime) - m_BaseTime) * m_SecondsPerCount);
	}
}
