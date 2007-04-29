
#include "timer.h"

#include "const.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

using namespace std;

static long long cpc() {
  return SDL_GetTicks();
};

static long long cpf() {
  return 1000;
};

void Timer::waitForNextFrame() {
  while(cpc() < frameNum * ticksPerFrame + ticksOffset)
    SDL_Delay(1);
};

bool Timer::skipFrame() {
  return cpc() > (frameNum + 2) * ticksPerFrame + ticksOffset;
};

int Timer::framesBehind() {
  return (cpc() - ticksOffset) / ticksPerFrame - frameNum;
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
