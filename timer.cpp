
#include "timer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "const.h"
#include "debug.h"

static long long cpc() {
	LARGE_INTEGER rv;
	QueryPerformanceCounter( &rv );
	return rv.QuadPart;
};

static long long cpf() {
	LARGE_INTEGER rv;
	QueryPerformanceFrequency( &rv );
	return rv.QuadPart;
};

void Timer::waitForNextFrame() {
	while( cpc() < frameNum * ticksPerFrame + ticksOffset )
		;
};

bool Timer::skipFrame() {
	return cpc() > ( frameNum + 2 ) * ticksPerFrame + ticksOffset;
};

void Timer::frameDone() {
	frameNum++;
};

long long Timer::ticksElapsed() {
	return cpc() - ticksOffset;
};

long long Timer::getFrameTicks() {
	return ticksPerFrame;
};


Timer::Timer() {
	ticksOffset = cpc();
	ticksPerFrame = cpf() / FPS;
	frameNum = 0;
};
