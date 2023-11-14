#include "Pch.h"
#include "MrClock.h"
#include <Windows.h>

RoveTimer::RoveTimer()
{
	__int64 counts_per_second = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);
	m_SecondsPerCount = 1.0 / static_cast<double>(counts_per_second);

	Reset();
}

void RoveTimer::Start()
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

void RoveTimer::Stop()
{
	if (!m_Stopped)
	{
		__int64 current_time = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

		m_StopTime = current_time;
		m_Stopped = true;
	}
}

void RoveTimer::Reset()
{
	__int64 current_time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

	m_BaseTime = current_time;
	m_PreviousTime = current_time;
	m_StopTime = 0;
	m_Stopped = false;

	Tick();
}

void RoveTimer::Tick()
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

double RoveTimer::DeltaTime()
{
	return static_cast<double>(m_DeltaTime);
}

double RoveTimer::TotalTime()
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
