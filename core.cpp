
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

DEFINE_bool(frameskip, true, "Enable or disable frameskipping");
DEFINE_bool(render, true, "Render shit");
DEFINE_bool(timing, true, "Display timing information");

long long polling = 0;
long long waiting = 0;
long long ticking = 0;
long long rendering = 0;

void displayCZInfo();

void MainLoop() {

  Timer timer;

  bool quit = false;

  int frako = 0;
  
  Rng rng(unsync().generate_seed());
  pair<RngSeed, vector<Controller> > rc = controls_init(rng.generate_seed());
  RngSeed game_seed = rc.first;
  vector<Controller> controllers = rc.second;
  
  vector<Controller> origcontrollers = controllers;
  
  dprintf("Final controllers:");
  for(int i = 0; i < controllers.size(); i++) {
    dprintf("%d: %d buttons, %d axes", i, controllers[i].keys.size(), controllers[i].axes.size());
  }
  
  FILE *outfile = NULL;
  
  int skipped = 0;
  
  frameNumber = 0;    // it's -1 before this point
  
  InterfaceMain interface;

  time_t starttime = time(NULL);

  Timer bencher;
  
  while(!quit) {
    startPerformanceBar();
    if(FLAGS_timing)
      bencher = Timer();
    StackString sst(StringPrintf("Frame %d loop", frameNumber));
    tickHttpd();
    ffwd = (frameNumber < FLAGS_fastForwardTo);
    if(frameNumber == FLAGS_fastForwardTo)
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
          if(event.key.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            quit = true;
          controls_key(&event.key);
          break;

        case SDL_VIDEORESIZE:
          CHECK(0);
          //CreateWindow("Destruction Net", event.resize.w, event.resize.h);
          break;

        default:
          break;
      }
    }
    if(quit || FLAGS_terminateAfter != -1 && time(NULL) - starttime >= FLAGS_terminateAfter)
      break;
    if(FLAGS_terminateAfterFrame != -1 && FLAGS_terminateAfterFrame <= frameNumber)
      break;
    interface.ai(controls_ai());  // has to be before controls
    controllers = controls_next();
    if(!controllers.size())
      break;
    CHECK(controllers.size() == origcontrollers.size());
    for(int i = 0; i < controllers.size(); i++)
      CHECK(controllers[i].keys.size() == origcontrollers[i].keys.size());
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
          int dat = 3;
          fwrite(&dat, 1, sizeof(dat), outfile);
          fwrite(&game_seed, 1, sizeof(game_seed), outfile);  // this is kind of grim and nasty
          dat = controllers.size();
          fwrite(&dat, 1, sizeof(dat), outfile);
          for(int i = 0; i < controllers.size(); i++) {
            dat = controllers[i].keys.size();
            fwrite(&dat, 1, sizeof(dat), outfile);
            dat = controllers[i].axes.size();
            fwrite(&dat, 1, sizeof(dat), outfile);
          }
          fflush(outfile);
        } else {
          dprintf("Outfile %s couldn't be opened! Not writing dump", fname.c_str());
        }
      }
      if(outfile) {
        for(int i = 0; i < controllers.size(); i++) {
          fwrite(&controllers[i].menu.x, 1, sizeof(controllers[i].menu.x), outfile);
          fwrite(&controllers[i].menu.y, 1, sizeof(controllers[i].menu.y), outfile);
          fwrite(&controllers[i].u.down, 1, sizeof(controllers[i].u.down), outfile);
          fwrite(&controllers[i].d.down, 1, sizeof(controllers[i].d.down), outfile);
          fwrite(&controllers[i].l.down, 1, sizeof(controllers[i].l.down), outfile);
          fwrite(&controllers[i].r.down, 1, sizeof(controllers[i].r.down), outfile);
          for(int j = 0; j < controllers[i].keys.size(); j++)
            fwrite(&controllers[i].keys[j].down, 1, sizeof(controllers[i].keys[j].down), outfile);
          for(int j = 0; j < controllers[i].axes.size(); j++)
            fwrite(&controllers[i].axes[j], 1, sizeof(controllers[i].axes[j]), outfile);
        }
        fflush(outfile);
      }
    }
    if(FLAGS_timing) {
      polling += bencher.ticksElapsed();
      bencher = Timer();
    }
    {
      PerfStack pst(PBC::tick);
      if(interface.tick(controllers, game_seed))
        quit = true;
    }
    if(FLAGS_timing) {
      ticking += bencher.ticksElapsed();
      bencher = Timer();
    }
    if(frameNumber >= FLAGS_fastForwardTo) {
      timer.waitForNextFrame();
    }
    if(FLAGS_timing) {
      waiting += bencher.ticksElapsed();
      bencher = Timer();
    }
    if(FLAGS_render && (!FLAGS_frameskip || frameNumber % (ffwd ? 60 : 6) == 0 || (!ffwd || frameNumber % 60 == 0) && !timer.skipFrame())) {
      initFrame();
      interface.render();
      if(!controls_users()) {
        setColor(1.0, 1.0, 1.0);
        setZoom(Float4(0, 0, 133.333, 100));
        drawText(StringPrintf("%d", frameNumber), 10, Float2(5, 85));
      }
      drawPerformanceBar();
      deinitFrame();
      SDL_GL_SwapBuffers();
    } else {
      skipped++;
    }
    if(FLAGS_timing) {
      rendering += bencher.ticksElapsed();
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
        dprintf("%d frames in %ld seconds, %.2fx overall (%.2f hours gametime)", frameNumber, time(NULL) - starttime, (frameNumber / 60.) / (time(NULL) - starttime), frameNumber / 60. / 60 / 60);
        polling = 0;
        ticking = 0;
        waiting = 0;
        rendering = 0;
        skipped = 0;
      }
    }
    frameNumber++;
    frako++;
  }
  dprintf("Control shutdown\n");
  controls_shutdown();
  dprintf("Exiting core\n");
}
