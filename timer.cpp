
#include "timer.h"

#include "const.h"
#include "debug.h"
#include "os.h"

#ifndef NOSDL
  #ifdef OSX_FRAMEWORK_PREFIXES
    #include <SDL/SDL.h>
  #else
    #include <SDL.h>
  #endif
#endif

using namespace std;

#ifdef NO_WINDOWS

static long long cpc() {
  return SDL_GetTicks();
};

static long long cpf() {
  return 1000;
};

#else

#include <windows.h>

static long long cpc() {
  LARGE_INTEGER li;
  CHECK(QueryPerformanceCounter(&li));
  return li.QuadPart;
};

static long long cpf() {
  LARGE_INTEGER li;
  CHECK(QueryPerformanceFrequency(&li));
  return li.QuadPart;
};

#endif

#ifndef NOSDL
  void Timer::waitForNextFrame() {
    while(cpc() < frameNum * ticksPerFrame + ticksOffset)
      SDL_Delay(1);
  };
#else
  void Timer::waitForNextFrame() {
    while(cpc() < frameNum * ticksPerFrame + ticksOffset)
      ;
  }
#endif

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
