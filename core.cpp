
#include "core.h"

#include "args.h"
#include "debug.h"
#include "gfx.h"
#include "inputsnag.h"
#include "interface.h"
#include "rng.h"
#include "timer.h"
#include "util.h"
#include "httpd.h"
#include "perfbar.h"
#include "adler32.h"
#include "dumper.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

using namespace std;

DEFINE_int(fastForwardTo, 0, "Fastforward rendering to this frame");
DEFINE_int(terminateAfterFrame, -1, "Terminate execution after this many frames");
DEFINE_int(terminateAfter, -1, "Terminate execution after this many seconds");
DEFINE_int(randomizeFrameRender, 0, "Randomize frame render change to 1/this (default 0 for disabled)");

DEFINE_bool(frameskip, true, "Enable or disable frameskipping");
DEFINE_bool(render, true, "Render shit");
DEFINE_bool(timing, true, "Display timing information");
DEFINE_bool(warpkeys, false, "Enable timewarp keys");
DEFINE_bool(renderframenumber, true, "Render frame number when AI is on");

DEFINE_bool(checksumGameState, true, "Checksum the game state on every frame");

void displayCZInfo();

void MainLoop() {

  Timer timer;

  bool quit = false;

  int frako = 0;
  
  long long polling = 0;
  long long waiting = 0;
  long long ticking = 0;
  long long rendering = 0;
  int adlers = 0;
  
  Rng rng(unsync().generate_seed());
  
  RngSeed game_seed = dumper_init(rng.generate_seed());
  
  InputState is = controls_init();
  ControlShutdown csd;
  
  dumper_read_adler();
  
  InputState origis = is;
  
  dprintf("Final controllers:");
  for(int i = 0; i < is.controllers.size(); i++) {
    dprintf("%d: %d buttons, %d axes", i, is.controllers[i].keys.size(), is.controllers[i].axes.size());
  }
  
  int skipped = 0;
  
  frameNumber = 0;    // it's -1 before this point

  InterfaceMain interface;

  reg_adler_pause();
  
  time_t starttime = time(NULL);

  Timer bencher;
  
  int speed = 1;
  
  // This all needs to be commented much better.
  while(!quit) {
    bool thistick = false;
    
    startPerformanceBar();
    if(FLAGS_timing)
      bencher = Timer();
    StackString sst(StringPrintf("Frame %d loop", frameNumber));
    tickHttpd();
    ffwd = (frameNumber < FLAGS_fastForwardTo);
    if(frameNumber == FLAGS_fastForwardTo || speed != 1)
      timer = Timer();    // so we don't end up sitting there for aeons waiting for another frame
    if(FLAGS_timing)
      bencher = Timer();
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_QUIT:
          quit = true;
          break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
          if(FLAGS_warpkeys && event.key.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z)
            speed = !speed;
          if(FLAGS_warpkeys && event.key.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_x)
            speed = 10;
          if(FLAGS_warpkeys && event.key.type == SDL_KEYUP && event.key.keysym.sym == SDLK_x)
            speed = 0;
          if(FLAGS_warpkeys && event.key.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c)
            thistick = true;
          if(FLAGS_warpkeys && event.key.type == SDL_KEYUP && event.key.keysym.sym == SDLK_c)
            speed = 0;
          
          controls_key(&event.key);
          break;

        case SDL_VIDEORESIZE:
          interface.forceResize(event.resize.w, event.resize.h);
          break;

        default:
          break;
      }
    }
    if(quit || FLAGS_terminateAfter != -1 && time(NULL) - starttime >= FLAGS_terminateAfter)
      break;
    if(FLAGS_terminateAfterFrame != -1 && FLAGS_terminateAfterFrame <= frameNumber)
      break;
    {
      int tspeed = speed;
      if(thistick && !tspeed)
        tspeed = 1;
      for(int i = 0; i < tspeed; i++) {
        
        interface.ai(controls_ai());  // has to be before controls
        
        reg_adler_unpause();
        dumper_write_adler();
        adlers += reg_adler_read_count();
        is = controls_next();
        if(!is.valid)
          return;
        dumper_write_input(is);
        dumper_read_adler();
        
        CHECK(is.controllers.size() == origis.controllers.size());
        for(int i = 0; i < is.controllers.size(); i++)
          CHECK(is.controllers[i].keys.size() == origis.controllers[i].keys.size());
        
        {
          Adler32 adl;
          PerfStack pst(PBC::checksum);
          reg_adler_intermed(adl);
          adler(&adl, frameNumber);
          reg_adler_intermed(adl);
          if(FLAGS_checksumGameState) {
            reg_adler_intermed(adl);
            interface.checksum(&adl);
            reg_adler_intermed(adl);
          }
          reg_adler(adl);
        }
        
        reg_adler_ul(0);  // so we have one item, and for rechecking's sake
        
        if(FLAGS_timing) {
          polling += bencher.ticksElapsed();
          bencher = Timer();
        }
        {
          PerfStack pst(PBC::tick);
          if(interface.tick(is, game_seed))
            quit = true;
        }
        if(FLAGS_timing) {
          ticking += bencher.ticksElapsed();
          bencher = Timer();
        }
        
        frameNumber++;
        
        reg_adler_pause();
      }
    }
    if(FLAGS_render) {
      bool render = false;
      if(FLAGS_randomizeFrameRender != 0) {
        CHECK(FLAGS_randomizeFrameRender > 0);
        render = unsync().frand() < (1. / FLAGS_randomizeFrameRender);
      } else if(ffwd) {
        render = (frameNumber % 60 == 0);
      } else {
        render = (frameNumber % 6 == 0) || !timer.skipFrame();
      }
      
      if(speed != 1)
        render = true;
      
      if(render) {
        {
          PerfStack pst(PBC::render);
          {
            PerfStack pst(PBC::renderinit);
            initFrame();
          }
          interface.render();
          if(!controls_users() && FLAGS_renderframenumber) {
            setColor(1.0, 1.0, 1.0);
            setZoomVertical(0, 0, 1);
            drawText(StringPrintf("%d", frameNumber), 10, Float2(5, 85));
          }
        }
        drawPerformanceBar();
        deinitFrame();
        SDL_GL_SwapBuffers();
      } else {
        skipped++;
      }
    }
    
    if(FLAGS_timing) {
      rendering += bencher.ticksElapsed();
      bencher = Timer();
    }
    if(frameNumber >= FLAGS_fastForwardTo) {
      timer.waitForNextFrame();
    }
    if(FLAGS_timing) {
      waiting += bencher.ticksElapsed();
      bencher = Timer();
    }
    timer.frameDone();
    {
      int frameSplit;
      if(ffwd)
        frameSplit = 600;
      else
        frameSplit = 60;
      if(frako % frameSplit == 0) {
        long long tot = timer.getFrameTicks() * frameSplit;
        //long long tot = polling + ticking + waiting + rendering;
        dprintf("%4d polling", int(polling * 1000 / tot));
        dprintf("%4d ticking", int(ticking * 1000 / tot));
        dprintf("%4d waiting", int(waiting * 1000 / tot));
        dprintf("%4d rendering", int(rendering * 1000 / tot));
        dprintf("%4d skipped", skipped);
        dprintf("    %s", printGraphicsStats().c_str());
        displayCZInfo();
        dprintf("    %d adlers per frame", adlers / frameSplit);
        dprintf("%d frames in %ld seconds, %.2fx overall (%.2f hours gametime)", frameNumber, time(NULL) - starttime, (frameNumber / 60.) / (time(NULL) - starttime), frameNumber / 60. / 60 / 60);
        polling = 0;
        ticking = 0;
        waiting = 0;
        rendering = 0;
        skipped = 0;
        adlers = 0;
      }
    }
    frako++;
  }
  
  reg_adler_unpause();
  
  dumper_write_adler();
  
  dumper_shutdown();
}
