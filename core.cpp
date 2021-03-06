
#include "core.h"

#include "audit.h"
#include "dumper_registry.h"
#include "gfx.h"
#include "httpd.h"
#include "interface.h"
#include "os_timer.h"
#include "perfbar.h"

using namespace std;

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif


DEFINE_int(fastForwardTo, 0, "Fastforward rendering to this frame");
DEFINE_int(terminateAfterFrame, -1, "Terminate execution after this many frames");
DEFINE_int(terminateAfter, -1, "Terminate execution after this many seconds");
DEFINE_int(randomizeFrameRender, 0, "Randomize frame render change to 1/this (default 0 for disabled)");

DEFINE_bool(frameskip, true, "Enable or disable frameskipping");
DEFINE_bool(render, true, "Render toggle");
DEFINE_bool(timing, true, "Display timing information");
DEFINE_bool(warpkeys, false, "Enable timewarp keys");
DEFINE_bool(renderframenumber, true, "Render frame number when AI is on");

DEFINE_bool(checksumGameState, true, "Checksum the game state on every frame");

DECLARE_bool(hideAiShopping);

DEFINE_int(aiCount, 0, "AI count for full automation");
REGISTER_int(aiCount);

void displayCZInfo();

void MainLoop() {
  StackString sst("MainLoop");

  Timer timer;

  int frako = 0;
  
  long long polling = 0;
  long long waiting = 0;
  long long ticking = 0;
  long long rendering = 0;
  int adlers = 0;
  
  Rng rng(unsync().generate_seed());
  
  Dumper dumper;
  
  RngSeed game_seed = dumper.prepare(rng.generate_seed());
  
  frameNumber = 0;    // it's -1 before this point
  
  InputSnag isnag;
  InputState is = isnag.init(&dumper, FLAGS_aiCount == 0, FLAGS_aiCount);
  
  dumper.read_audit();
  
  dprintf("Final controllers:");
  for(int i = 0; i < is.controllers.size(); i++) {
    dprintf("%d: %d buttons, %d axes", i, is.controllers[i].keys.size(), is.controllers[i].axes.size());
  }
  
  int skipped = 0;

  InterfaceMain interface(isnag);

  dumper.write_audit();
  
  time_t starttime = time(NULL);

  Timer bencher;
  
  int speed = 1;
  
  // This all needs to be commented much better.
  while(1) {
    StackString sst(StringPrintf("Frame %d loop", frameNumber));
    
    bool thistick = false;
    
    startPerformanceBar();
    if(FLAGS_timing)
      bencher = Timer();
    
    ffwd = (frameNumber < FLAGS_fastForwardTo);
    if(frameNumber == FLAGS_fastForwardTo || speed != 1)
      timer = Timer();    // so we don't end up sitting there for aeons waiting for another frame
    if(FLAGS_timing)
      bencher = Timer();
    
    {
      int frames = 0;
      do {
        StackString sst("Brain loop");
        {
          StackString sst("SDL events");
          SDL_Event event;
          while(SDL_PollEvent(&event)) {
            switch(event.type) {
              case SDL_QUIT:
                dprintf("Returning, SDL_QUIT\n");
                return;

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
                
                isnag.key(&event.key);
                break;
              
              case SDL_MOUSEBUTTONDOWN:
                isnag.mouseclick();
                break;
              
              case SDL_VIDEORESIZE:
                interface.forceResize(event.resize.w, event.resize.h);
                break;

              default:
                break;
            }
          }
        }
        
        if(FLAGS_terminateAfter != -1 && time(NULL) - starttime >= FLAGS_terminateAfter) {
          dprintf("Returning, %d seconds elapsed (%d allowed)\n", int(time(NULL) - starttime), FLAGS_terminateAfter);
          return;
        }
        if(FLAGS_terminateAfterFrame != -1 && FLAGS_terminateAfterFrame <= frameNumber) {
          dprintf("Returning, %d frames elapsed (%d allowed)\n", frameNumber, FLAGS_terminateAfterFrame);
          return;
        }
        
        {
          StackString sst("Brain core");
          
          int tspeed = speed;
          if(thistick && !tspeed)
            tspeed = 1;
          for(int i = 0; i < tspeed; i++) {
            if(dumper.is_done()) {
              dprintf("Returning, dumper is done\n");
              return;
            }
            
            if(dumper.has_checksum(dumper.is_writing())) {
              dumper.read_checksum_audit();
              Adler32 adl;
              PerfStack pst(PBC::checksum);
              adler(&adl, frameNumber);
              if(FLAGS_checksumGameState) {
                interface.checksum(&adl);
              }
              audit(adl);
              dumper.write_checksum_audit();
              adlers += audit_read_count();
            }
            
            tickHttpd();  // We do this here so we can inject stuff between control cycles. Yurgh.
            
            is = isnag.next(&dumper);
            
            dumper.write_input(is);
            
            dumper.read_audit();
            
            /*
            for(int i = 0; i < is.controllers.size(); i++) {
              for(int j = 0; j < is.controllers[i].keys.size(); j++)
                audit(is.controllers[i].keys[j].down);
              for(int j = 0; j < is.controllers[i].axes.size(); j++) {
                audit(is.controllers[i].axes[j].raw());
                audit(is.controllers[i].axes[j].raw() >> 32);
              }
              audit(is.controllers[i].menu.x.raw());
              audit(is.controllers[i].menu.x.raw() >> 32);
              audit(is.controllers[i].menu.y.raw());
              audit(is.controllers[i].menu.y.raw() >> 32);
            }
            */
            
            audit(0);  // so we have one item, and for rechecking's sake
            
            if(FLAGS_timing) {
              polling += bencher.ticksElapsed();
              bencher = Timer();
            }
            {
              smart_ptr<AudioShifter> as;
              if(interface.isWaitingOnAi())
                as.reset(new AudioShifter(0.0, 0.0));
              
              PerfStack pst(PBC::tick);
              if(interface.tick(is, game_seed, isnag)) {
                dprintf("Returning, interface said so\n");
                dumper.write_audit();
                return;
              }
            }
            if(FLAGS_timing) {
              ticking += bencher.ticksElapsed();
              bencher = Timer();
            }
            
            frameNumber++;
            
            dumper.write_audit();
            adlers += audit_read_count();
            
            interface.ai(isnag.ais(), isnag.human_flags());  // We do this afterwards so we don't delay until our next tick is starting.
          }
        }
        
        frames++;
      } while(interface.isWaitingOnAi() && frames < FPS * 3 && FLAGS_hideAiShopping);
      
      if(frames > 1) {
        dprintf("Finished %d-frame AI chunk\n", frames);
        timer = Timer();
      }
    }
    
    //if(frameNumber % 15 == 0)
      //timer.printStats();
    
    bool render = false;
    if(FLAGS_render) {
      StackString sst("Render test");
      
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
    }
  
    {
      if(render) {
        StackString sst("Render");
        {
          {
            PerfStack pst(PBC::rinit);
            initFrame();
          }
          PerfStack pst(PBC::render);
          interface.render(isnag);
          if(!isnag.users() && FLAGS_renderframenumber) {
            setColor(1.0, 1.0, 1.0);
            setZoomVertical(0, 0, 1);
            drawText(StringPrintf("%d", frameNumber), 10, Float2(5, 85));
          }
        }
        flushFrame();
      } else {
        skipped++;
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
      
      if(render) {
        drawPerformanceBar();
        deinitFrame();
        SDL_GL_SwapBuffers();
      }
    }
    
    timer.frameDone();
    {
      StackString sst("Bookkeeping");
      
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
  
  CHECK(0); // Returns from this function
}
