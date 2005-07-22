
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

vector< Keystates > curstates( 2 ); // these are not kept in a useful state


void keyPress( SDL_KeyboardEvent *key ) {
	bool *ps = NULL;
	if( key->keysym.sym == SDLK_UP )
		ps = &curstates[0].u.down;
	if( key->keysym.sym == SDLK_DOWN )
		ps = &curstates[0].d.down;
	if( key->keysym.sym == SDLK_LEFT )
		ps = &curstates[0].l.down;
	if( key->keysym.sym == SDLK_RIGHT )
		ps = &curstates[0].r.down;
	if( key->keysym.sym == SDLK_z )
		ps = &curstates[0].f.down;
	if( key->keysym.sym == SDLK_w )
		ps = &curstates[1].u.down;
	if( key->keysym.sym == SDLK_s )
		ps = &curstates[1].d.down;
	if( key->keysym.sym == SDLK_a )
		ps = &curstates[1].l.down;
	if( key->keysym.sym == SDLK_d )
		ps = &curstates[1].r.down;
	if( key->keysym.sym == SDLK_x )
		ps = &curstates[1].f.down;
	if( !ps )
		return;
	if( key->type == SDL_KEYUP )
		*ps = 0;
	if( key->type == SDL_KEYDOWN )
		*ps = 1;
}

vector< Controller > mungeToControllers(const vector<Keystates> &curstates) {
    vector< Controller > controllers( 2 );
    for(int i = 0; i < 2; i++) {
        controllers[i].x = curstates[i].r.down - curstates[i].l.down;
        controllers[i].y = curstates[i].u.down - curstates[i].d.down;
        controllers[i].keys.resize(1);
        controllers[i].keys[0] = curstates[i].f.down;
    }
    return controllers;
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
    
    vector<Keystates> fullstates(2);
    vector<Controller> controllers(2);
    
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
        CHECK(fullstates.size() == curstates.size());
        for(int i = 0; i < fullstates.size(); i++) {
            fullstates[i].u.newState(curstates[i].u.down);
            fullstates[i].d.newState(curstates[i].d.down);
            fullstates[i].l.newState(curstates[i].l.down);
            fullstates[i].r.newState(curstates[i].r.down);
            fullstates[i].f.newState(curstates[i].f.down);
        }
        controllers = mungeToControllers(fullstates);
		polling += bencher.ticksElapsed();
		bencher = Timer();
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
        }
        if(interfaceRunTick( controllers, fullstates[0] ))
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
