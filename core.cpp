
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

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

using namespace std;

DEFINE_string(writeTarget, "dumps/dump", "Prefix for file dump");
//DEFINE_int(writeTargetCheckpoint, 2000000000, "Write target checkpoint frequency"); // currently disabled

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
  
  pair<RngSeed, InputState> rc = controls_init(rng.generate_seed());
  ControlShutdown csd;
  RngSeed game_seed = rc.first;
  InputState is = rc.second;
  
  InputState origis = is;
  
  dprintf("Final controllers:");
  for(int i = 0; i < is.controllers.size(); i++) {
    dprintf("%d: %d buttons, %d axes", i, is.controllers[i].keys.size(), is.controllers[i].axes.size());
  }
  
  FILE *outfile = NULL;
  
  int skipped = 0;
  
  frameNumber = 0;    // it's -1 before this point
  
  InterfaceMain interface;

  time_t starttime = time(NULL);

  Timer bencher;
  
  int speed = 1;
  
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
        is = controls_next();
        if(!is.valid)
          return;
        CHECK(is.controllers.size() == origis.controllers.size());
        for(int i = 0; i < is.controllers.size(); i++)
          CHECK(is.controllers[i].keys.size() == origis.controllers[i].keys.size());
        {
          Adler32 adl;
          PerfStack pst(PBC::checksum);
          adler(&adl, frameNumber);
          if(FLAGS_checksumGameState)
            interface.checksum(&adl);
          reg_adler(adl);
          adlers += ret_adler_ref_count();
        }
        
        if(FLAGS_writeTarget != "" && controls_recordable()) {
          if(frameNumber == 0) {
            if(outfile)
              fclose(outfile);
            
            string fname = FLAGS_writeTarget;
            char timestampbuf[128];
            time_t ctmt = time(NULL);
            strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S", gmtime(&ctmt));
            fname = StringPrintf("%s-%s-%010d.dnd", fname.c_str(), timestampbuf, frameNumber);
            dprintf("%s\n", fname.c_str());
            outfile = fopen(fname.c_str(), "wb");
            if(outfile) {
              int dat = 7;
              fwrite(&dat, 1, sizeof(dat), outfile);
              fwrite(&game_seed, 1, sizeof(game_seed), outfile);  // this is kind of grim and nasty
              dat = is.controllers.size();
              fwrite(&dat, 1, sizeof(dat), outfile);
              for(int i = 0; i < is.controllers.size(); i++) {
                dat = is.controllers[i].keys.size();
                fwrite(&dat, 1, sizeof(dat), outfile);
                dat = is.controllers[i].axes.size();
                fwrite(&dat, 1, sizeof(dat), outfile);
              }
              fflush(outfile);
            } else {
              dprintf("Outfile %s couldn't be opened! Not writing dump", fname.c_str());
            }
          }
          if(outfile) {
            fwrite(&is.escape.down, 1, sizeof(is.escape.down), outfile);
            for(int i = 0; i < is.controllers.size(); i++) {
              fwrite(&is.controllers[i].menu.x, 1, sizeof(is.controllers[i].menu.x), outfile);
              fwrite(&is.controllers[i].menu.y, 1, sizeof(is.controllers[i].menu.y), outfile);
              fwrite(&is.controllers[i].u.down, 1, sizeof(is.controllers[i].u.down), outfile);
              fwrite(&is.controllers[i].d.down, 1, sizeof(is.controllers[i].d.down), outfile);
              fwrite(&is.controllers[i].l.down, 1, sizeof(is.controllers[i].l.down), outfile);
              fwrite(&is.controllers[i].r.down, 1, sizeof(is.controllers[i].r.down), outfile);
              for(int j = 0; j < is.controllers[i].keys.size(); j++)
                fwrite(&is.controllers[i].keys[j].down, 1, sizeof(is.controllers[i].keys[j].down), outfile);
              for(int j = 0; j < is.controllers[i].axes.size(); j++)
                fwrite(&is.controllers[i].axes[j], 1, sizeof(is.controllers[i].axes[j]), outfile);
            }
    
            int refc = ret_adler_ref_count();
            fwrite(&refc, 1, sizeof(refc), outfile);
            for(int i = 0; i < refc; i++) {
              unsigned long ref = ret_adler_ref();
              fwrite(&ref, 1, sizeof(ref), outfile);
            }
            fflush(outfile);
          }
        }
        
        ret_adler_ref_clear();
        
        controls_snag_next_checksum_set();
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
        
        adlers += ret_adler_ref_count();
        if(outfile) {
          int refc = ret_adler_ref_count();
          fwrite(&refc, 1, sizeof(refc), outfile);
          for(int i = 0; i < refc; i++) {
            unsigned long ref = ret_adler_ref();
            fwrite(&ref, 1, sizeof(ref), outfile);
          }
          ret_adler_ref_clear();
          fflush(outfile);
        }
        
        frameNumber++;
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
            setZoom(Float4(0, 0, 133.333, 100));
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
}
