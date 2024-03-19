#pragma once

#include <cstdint>

// High resolution timer - https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
class HighTimer
{
public:
	HighTimer();
	virtual ~HighTimer() = default;

	// Starts the timer
	void Start();

	// Stops the timer
	void Stop();

	// Resets the timer
	void Reset();

	// Calculate the time since last called
	void Tick();

	// Gets the time between frame
	double DeltaTime();

	// Gets the total time since started
	double TotalTime();

private:
	double m_SecondsPerCount = 0.0;
	double m_DeltaTime = 0.0;

	uint64_t  m_BaseTime = 0;
	uint64_t m_PausedTime = 0;
	uint64_t m_StopTime = 0;
	uint64_t m_PreviousTime = 0;
	uint64_t m_CurrentTime = 0;

	bool m_Stopped = false;
};