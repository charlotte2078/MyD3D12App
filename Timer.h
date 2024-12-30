// Timer class - needed for frame stats
// Based on Frank Luna's code from 3D Game programming with DirectX 12 pp. 131-139

#pragma once

#include <Windows.h>

class Timer
{
public:
	// Constructors
	Timer();

	// Getters
	float GetTotalTime() const; // in Seconds
	float GetDeltaTime() const; // in Seconds

	// Timer control functions
	void Reset();	// Call before message loop
	void Start();	// Call when unpaused
	void Stop();	// Call when paused
	void Tick();	// Call every frame

private:
	// Frequency and time between frames
	double mSecondsPerCount;
	double mDeltaTime;

	// Large integers used for precision
	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	// To store the timer's state
	bool mIsStopped;
};
