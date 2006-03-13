
#include <iostream>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "core.h"
#include "main.h"
#include "interface.h"
#include "timer.h"
#include "debug.h"
#include "gfx.h"
#include "util.h"
#include "args.h"
#include "rng.h"
#include "inputsnag.h"

DEFINE_string(writeTarget, "data/dump", "Prefix for file dump");
DEFINE_int(writeTargetCheckpoint, 2000000000, "Write target checkpoint frequency"); // currently disabled

DEFINE_int(fastForwardTo, 0, "Fastforward rendering to this frame");

DEFINE_bool(frameskip, true, "Enable or disable frameskipping");

long long polling = 0;
long long waiting = 0;
long long ticking = 0;
long long rendering = 0;

void MainLoop() {

  sfrand(time(NULL));

  interfaceInit();

	Timer timer;
	Timer bencher;

	bool quit = false;

	int frako = 0;

  vector<Controller> controllers = controls_init();
  vector<Controller> origcontrollers = controllers;
  
  dprintf("Final controllers:");
  for(int i = 0; i < controllers.size(); i++) {
      dprintf("%d: %d buttons, %d axes", i, controllers[i].keys.size(), controllers[i].axes.size());
  }
  
  FILE *outfile = NULL;
  
  int skipped = 0;
  
  frameNumber = 0;    // it's -1 before this point
    
	while( !quit ) {
    StackString sst(StringPrintf("Frame %d loop", frameNumber));
    ffwd = ( frameNumber < FLAGS_fastForwardTo );
    if(frameNumber == FLAGS_fastForwardTo)
      timer = Timer();    // so we don't end up sitting there for aeons waiting for another frame
		bencher = Timer();
		SDL_Event event;
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
        case SDL_QUIT:
					quit = true;
          break;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
          if(event.key.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            quit = true;
					controls_key( &event.key );
					break;

				case SDL_VIDEORESIZE:
					CHECK( 0 );
					//CreateWindow( "Destruction Net", event.resize.w, event.resize.h );
					break;

				default:
					break;
			}
		}
		if(quit)
      break;
		controllers = controls_next();
		CHECK(controllers.size() == origcontrollers.size());
		for(int i = 0; i < controllers.size(); i++)
			CHECK(controllers[i].keys.size() == origcontrollers[i].keys.size());
    if(FLAGS_writeTarget != "" && controls_recordable()) {
      if(frameNumber % FLAGS_writeTargetCheckpoint == 0) {
        if(outfile)
          fclose(outfile);
        
        string fname = FLAGS_writeTarget;
        char timestampbuf[ 128 ];
        time_t ctmt = time(NULL);
        strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S.dnd", gmtime(&ctmt));
        fname = StringPrintf("%s-%010d-%s", fname.c_str(), frameNumber, timestampbuf);
        dprintf("%s\n", fname.c_str());
        outfile = fopen(fname.c_str(), "wb");
        if(outfile) {
          int dat = 3;
          fwrite(&dat, 1, sizeof(dat), outfile);
          dat = frandseed();
          fwrite(&dat, 1, sizeof(dat), outfile);
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
          fwrite(&controllers[i].x, 1, sizeof(controllers[i].x), outfile);
          fwrite(&controllers[i].y, 1, sizeof(controllers[i].y), outfile);
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
		polling += bencher.ticksElapsed();
		bencher = Timer();
    if(interfaceRunTick( controllers ))
      quit = true;
    interfaceRunAi(controls_ai());
		ticking += bencher.ticksElapsed();
		bencher = Timer();
    if(frameNumber >= FLAGS_fastForwardTo) {
      timer.waitForNextFrame();
    }
		waiting += bencher.ticksElapsed();
		bencher = Timer();
		if(!timer.skipFrame() && (!ffwd || frameNumber % 60 == 0) || !FLAGS_frameskip || frameNumber % 60 == 0) {
			initFrame();
			interfaceRenderToScreen();
      if(!controls_users()) {
        setColor(1.0, 1.0, 1.0);
        setZoom(0, 0, 100);
        drawText(StringPrintf("%d", frameNumber), 10, 5, 85);
      }
			deinitFrame();
		} else {
      skipped++;
		}
		rendering += bencher.ticksElapsed();
		timer.frameDone();
		frameNumber++;
    {
      int frameSplit;
      if(ffwd)
        frameSplit = 600;
      else
        frameSplit = 60;
      if( frako % frameSplit == 0 ) {
        long long tot = timer.getFrameTicks() * frameSplit;
        //long long tot = polling + ticking + waiting + rendering;
        dprintf( "%4d polling", int( polling * 1000 / tot ) );
        dprintf( "%4d ticking", int( ticking * 1000 / tot ) );
        dprintf( "%4d waiting", int( waiting * 1000 / tot ) );
        dprintf( "%4d rendering", int( rendering * 1000 / tot ) );
        dprintf( "%4d skipped", skipped );
        dprintf( "%f clusters/frame last %d frames", getAccumulatedClusterCount() / float(frameSplit - skipped), frameSplit);
        polling = 0;
        ticking = 0;
        waiting = 0;
        rendering = 0;
        skipped = 0;
      }
    }
    frako++;
	}
  dprintf("Control shutdown\n");
  controls_shutdown();
  dprintf("Exiting core\n");
}
