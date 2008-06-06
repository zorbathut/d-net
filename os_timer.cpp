
#include "os_timer.h"

#include "const.h"
#include "debug.h"
#include "os.h"
#include "util.h"

using namespace std;

#if defined(NO_WINDOWS) && defined(NOSDL)

// we resort to wx

#include <wx/timer.h>

static long long cpc() {
  return wxGetLocalTimeMillis().GetValue();
};

static long long cpf() {
  return 1000;
};

void delay() {
  wxSleep(0);
}

#elif defined(NO_WINDOWS)

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

static long long cpc() {
  return SDL_GetTicks();
};

static long long cpf() {
  return 1000;
};

void delay() {
  SDL_Delay(0);
}

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

void delay() {
}

#endif

void Timer::waitForNextFrame() {
  while(cpc() < frameNum * ticksPerFrame + ticksOffset)
    delay();
};

bool Timer::skipFrame() {
  return cpc() > (frameNum + 1) * ticksPerFrame + ticksOffset;
};

int Timer::framesBehind() {
  return (cpc() - ticksOffset) / ticksPerFrame - frameNum;
}

void Timer::frameDone() {/*
  long long diff = (cpc() - ticksOffset) - ticksPerFrame * frameNum;
  //if(frameNum % 15 == 0)
    //dprintf("diff is %lld\n", diff);
  ticksOffset = approach(ticksOffset, ticksOffset + diff, ticksPerFrame / 50);*/
  if(cpc() - ticksOffset - ticksPerFrame * frameNum > ticksPerFrame * 30) {
    // If we're more than 30 seconds behind, just skip up ahead.
    ticksOffset = cpc() - ticksPerFrame * frameNum;
  }
  frameNum++;
  
};

long long Timer::ticksElapsed() {
  return cpc() - ticksOffset;
};

long long Timer::getFrameTicks() {
  return ticksPerFrame;
};

void Timer::printStats() const {
  long long pos = cpc() - ticksOffset;
  dprintf("%lld offset, %lld relative to current frame", pos, pos - frameNum * ticksPerFrame);
}


Timer::Timer() {
  ticksOffset = cpc();
  ticksPerFrame = cpf() / FPS;
  frameNum = 0;
};
