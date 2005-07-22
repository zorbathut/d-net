
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

DEFINE_bool( writeToFile, true, "Dump keypresses to file during game" );
DEFINE_string( writeTarget, "dumps/dump", "Prefix for file dump" );

DEFINE_bool( readFromFile, false, "Replay game from keypress dump" );
DEFINE_string( readTarget, "", "File to replay from" );

vector< Controller > curstates( 2 );

int playermap[2][10] = {
    { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_u, SDLK_i, SDLK_o, SDLK_j, SDLK_k, SDLK_l },
    { SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_r, SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h }
};

void keyPress( SDL_KeyboardEvent *key ) {
	bool *ps = NULL;
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 10; j++) {
            if(key->keysym.sym == playermap[i][j]) {
                if(j == 0)
                    ps = &curstates[i].u.down;
                else if(j == 1)
                    ps = &curstates[i].d.down;
                else if(j == 2)
                    ps = &curstates[i].l.down;
                else if(j == 3)
                    ps = &curstates[i].r.down;
                else
                    ps = &curstates[i].keys[j-4].down;
            }
        }
    }
	if( !ps )
		return;
	if( key->type == SDL_KEYUP )
		*ps = 0;
	if( key->type == SDL_KEYDOWN )
		*ps = 1;
}

long long polling = 0;
long long waiting = 0;
long long ticking = 0;
long long rendering = 0;

void MainLoop() {
    
    CHECK( !FLAGS_readFromFile ); // not yet implemented
    CHECK( !( FLAGS_readFromFile && FLAGS_readTarget == "" ) );
    
    FILE *outfile = NULL;
    FILE *infile = NULL;
    
    if(FLAGS_writeToFile) {
        string fname = FLAGS_writeTarget;
        char timestampbuf[ 128 ];
        time_t ctmt = time(NULL);
        strftime(timestampbuf, sizeof(timestampbuf), "-%Y%m%d-%H%M%S.dnd", gmtime(&ctmt));
        dprintf("%s\n", timestampbuf);
        fname += timestampbuf;
        outfile = fopen(fname.c_str(), "wb");
        if(outfile) {
            int ver = 0;
            fwrite(&ver, 1, sizeof(ver), outfile);
        } else {
            dprintf("Outfile %s couldn't be opened! Not writing dump.", fname.c_str());
        }
    }
    
    srand(time(NULL));
    
    interfaceInit();

	Timer timer;

	Timer bencher;

	bool quit = false;

	int frako = 0;
    
    for(int i = 0; i < curstates.size(); i++) {
        curstates[i].keys.resize(6);
    }
    
    vector<Controller> controllers = curstates;
    
	while( !quit ) {
		bencher = Timer();
		SDL_Event event;
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
			case SDL_QUIT:
					return;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					keyPress( &event.key );
					break;

				case SDL_VIDEORESIZE:
					CHECK( 0 );
					//CreateWindow( "Destruction Net", event.resize.w, event.resize.h );
					break;

				default:
					break;
			}
		}
        CHECK(controllers.size() == curstates.size());
        for(int i = 0; i < curstates.size(); i++) {
            curstates[i].x = curstates[i].r.down - curstates[i].l.down;
            curstates[i].y = curstates[i].u.down - curstates[i].d.down;
            controllers[i].newState(curstates[i]);
        }
		polling += bencher.ticksElapsed();
		bencher = Timer();
        /*
        if(outfile) {
            for(int i = 0; i < fullstates.size(); i++) {
                char obyte = 0;
                obyte = (obyte << 1) + (bool)fullstates[i].f.down;
                obyte = (obyte << 1) + (bool)fullstates[i].u.down;
                obyte = (obyte << 1) + (bool)fullstates[i].d.down;
                obyte = (obyte << 1) + (bool)fullstates[i].l.down;
                obyte = (obyte << 1) + (bool)fullstates[i].r.down;
                fwrite(&obyte, 1, 1, outfile);
            }
            fflush(outfile);
        }*/
        if(interfaceRunTick( controllers ))
            quit = true;
		ticking += bencher.ticksElapsed();
		bencher = Timer();
		timer.waitForNextFrame();
		waiting += bencher.ticksElapsed();
		bencher = Timer();
		//if( !timer.skipFrame() ) {
			initFrame();
			interfaceRenderToScreen();
			deinitFrame();
		//} else {
			//dprintf( "Skipped!\n" );
		//}
		rendering += bencher.ticksElapsed();
		timer.frameDone();
		frameNumber++;
		if( frako % 60 == 0 ) {
			long long tot = timer.getFrameTicks() * 60;
			//long long tot = polling + ticking + waiting + rendering;
			dprintf( "%4d polling", int( polling * 1000 / tot ) );
			dprintf( "%4d ticking", int( ticking * 1000 / tot ) );
			dprintf( "%4d waiting", int( waiting * 1000 / tot ) );
			dprintf( "%4d rendering", int( rendering * 1000 / tot ) );
			polling = 0;
			ticking = 0;
			waiting = 0;
			rendering = 0;
		}
        frako++;
	}
	dprintf( "denial\n" );
}
