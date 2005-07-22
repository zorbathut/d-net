
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

vector< Keystates > curstates( 2 );
vector< Controller > controllers( 2 );

void keyPress( SDL_KeyboardEvent *key ) {
	char *ps = NULL;
	if( key->keysym.sym == SDLK_UP )
		ps = &curstates[0].forward;
	if( key->keysym.sym == SDLK_DOWN )
		ps = &curstates[0].back;
	if( key->keysym.sym == SDLK_LEFT )
		ps = &curstates[0].left;
	if( key->keysym.sym == SDLK_RIGHT )
		ps = &curstates[0].right;
	if( key->keysym.sym == SDLK_z )
		ps = &curstates[0].firing;
	if( key->keysym.sym == SDLK_w )
		ps = &curstates[1].forward;
	if( key->keysym.sym == SDLK_s )
		ps = &curstates[1].back;
	if( key->keysym.sym == SDLK_a )
		ps = &curstates[1].left;
	if( key->keysym.sym == SDLK_d )
		ps = &curstates[1].right;
	if( key->keysym.sym == SDLK_x )
		ps = &curstates[1].firing;
	if( !ps )
		return;
	if( key->type == SDL_KEYUP )
		*ps = 0;
	if( key->type == SDL_KEYDOWN )
		*ps = 1;
}

void mungeToControllers() {
    for(int i = 0; i < 2; i++) {
        controllers[i].x = curstates[i].right - curstates[i].left;
        controllers[i].y = curstates[i].forward - curstates[i].back;
        controllers[i].keys.resize(1);
        controllers[i].keys[0] = curstates[i].firing;
    }
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
        mungeToControllers();
		polling += bencher.ticksElapsed();
		bencher = Timer();
        if(outfile) {
            for(int i = 0; i < curstates.size(); i++) {
                char obyte = 0;
                obyte = (obyte << 1) + (bool)curstates[i].firing;
                obyte = (obyte << 1) + (bool)curstates[i].forward;
                obyte = (obyte << 1) + (bool)curstates[i].back;
                obyte = (obyte << 1) + (bool)curstates[i].left;
                obyte = (obyte << 1) + (bool)curstates[i].right;
                fwrite(&obyte, 1, 1, outfile);
            }
            fflush(outfile);
        }
        if(interfaceRunTick( controllers, curstates[0] ))
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
