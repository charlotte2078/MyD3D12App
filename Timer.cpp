#include "Timer.h"

// Defualt constructor
Timer::Timer() :
	mSecondsPerCount(0.0),
	mDeltaTime(-1.0),
	mBaseTime(0),
	mPausedTime(0),
	mStopTime(0),
	mPrevTime(0),
	mCurrTime(0),
	mIsStopped(false)
{
	// Calculate the seconds per count using the frequency of the performance timer
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns total time elapsed since Reset() was called.
// Does not include any time when the timer is stopped.
float Timer::GetTotalTime() const
{
	// Do not want to include any paused time - so subtract this from the count
	if (mIsStopped)
	{
		return static_cast<float>((mStopTime - mBaseTime - mPausedTime) * mSecondsPerCount);
	}
	else
	{
		return static_cast<float>((mCurrTime - mBaseTime - mPausedTime) * mSecondsPerCount);
	}
}

// Returns the time between frames
float Timer::GetDeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

// Resets the timer
void Timer::Reset()
{
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

	mBaseTime = currentTime;
	mPrevTime = currentTime;
	mStopTime = 0;
	mIsStopped = false;
}

// Starts the timer after it has been stopped
// Does nothing if called when the timer is not stopped
void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (mIsStopped)
	{
		// Keep track of the paused time
		mPausedTime += startTime - mStopTime;

		mPrevTime = startTime;
		mStopTime = 0;
		mIsStopped = false;
	}
}

// Stop the timer
// Does nothing if the timer is already stopped
void Timer::Stop()
{
	if (!mIsStopped)
	{
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

		mStopTime = currentTime;
		mIsStopped = true;
	}
}

// Called every frame
void Timer::Tick()
{
	if (mIsStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	mCurrTime = currentTime;

	// Time difference between this frame and the previous.
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// Prepare for next frame.
	mPrevTime = mCurrTime;

	// Force nonnegative
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}


