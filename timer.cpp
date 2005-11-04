
#include "timer.h"

#include "const.h"
#include "debug.h"

#include "SDL.h"

static long long cpc() {
	return SDL_GetTicks();
};

static long long cpf() {
    return 1000;
};

void Timer::waitForNextFrame() {
	while( cpc() < frameNum * ticksPerFrame + ticksOffset )
		;
};

bool Timer::skipFrame() {
	return cpc() > ( frameNum + 2 ) * ticksPerFrame + ticksOffset;
};

int Timer::framesBehind() {
    return ( cpc() - ticksOffset ) / ticksPerFrame - frameNum;
}

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
