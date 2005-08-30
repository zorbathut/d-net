
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

DEFINE_bool( writeToFile, false, "Dump keypresses to file during game" );
DEFINE_string( writeTarget, "dumps/dump", "Prefix for file dump" );

DEFINE_bool( readFromFile, false, "Replay game from keypress dump" );
DEFINE_string( readTarget, "", "File to replay from" );

vector< Controller > curstates;

const int playerkeys = 9;

int baseplayermap[2][4 + playerkeys] = {
    { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_j },
    { SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_r, SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h, SDLK_v, SDLK_b, SDLK_n }
};

vector<vector<int> > playermap;

void keyPress( SDL_KeyboardEvent *key ) {
	bool *ps = NULL;
    for(int i = 0; i < playermap.size(); i++) {
        for(int j = 0; j < playermap[i].size(); j++) {
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
        CHECK(0);
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
    
    playermap.resize(2);
    
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < playerkeys + 4; j++) {
            playermap[i].push_back(baseplayermap[i][j]);
        }
    }
    
    playermap[0].push_back(SDLK_k);
    playermap[0].push_back(SDLK_l);
    playermap[0].push_back(SDLK_SEMICOLON);
    playermap[0].push_back(SDLK_m);
    playermap[0].push_back(SDLK_COMMA);
    playermap[0].push_back(SDLK_PERIOD);
    playermap[0].push_back(SDLK_SLASH);

    srand(time(NULL));
    
    interfaceInit();

	Timer timer;

	Timer bencher;

	bool quit = false;

	int frako = 0;
    
    dprintf("%d joysticks detected\n", SDL_NumJoysticks());
    
    curstates.resize(2 + SDL_NumJoysticks());
    
    vector<SDL_Joystick *> joysticks;
    for(int i = 0; i < SDL_NumJoysticks(); i++) {
        dprintf("Opening %d: %s\n", i, SDL_JoystickName(i));
        joysticks.push_back(SDL_JoystickOpen(i));
        CHECK(SDL_JoystickNumAxes(joysticks.back()) >= 2);
        CHECK(SDL_JoystickNumButtons(joysticks.back()) >= 1);
        dprintf("%d axes, %d buttons\n", SDL_JoystickNumAxes(joysticks.back()), SDL_JoystickNumButtons(joysticks.back()));
    }
    
    dprintf("Done opening joysticks\n");
    
    for(int i = 0; i < curstates.size(); i++) {
        if(i < 2) {
            curstates[i].keys.resize(playermap[i].size() - 4);
        } else {
            curstates[i].keys.resize(SDL_JoystickNumButtons(joysticks[i - 2]));
        }
    }
    
    vector<Controller> controllers = curstates;
    
    dprintf("Final controllers:");
    for(int i = 0; i < curstates.size(); i++) {
        dprintf("%d: %d buttons", i, curstates[i].keys.size());
    }
    
    int skipped = 0;
    
	while( !quit ) {
		bencher = Timer();
		SDL_Event event;
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
			case SDL_QUIT:
					quit = true;
                    break;

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
        if(quit)
            break;
        for(int i = 0; i < 2; i++) {
            curstates[i].x = curstates[i].r.down - curstates[i].l.down;
            curstates[i].y = curstates[i].u.down - curstates[i].d.down;
        }
        SDL_JoystickUpdate();
        for(int i = 0; i < joysticks.size(); i++) {
            int target = i + 2;
            curstates[target].x = SDL_JoystickGetAxis(joysticks[i], 0) / 32768.0f;
            curstates[target].y = -(SDL_JoystickGetAxis(joysticks[i], 1) / 32768.0f);
            if(abs(curstates[target].x) < .2)
                curstates[target].x = 0;
            if(abs(curstates[target].y) < .2)
                curstates[target].y = 0;
            if(curstates[target].x < -0.5) {
                curstates[target].r.down = false;
                curstates[target].l.down = true;
            } else if(curstates[target].x > 0.5) {
                curstates[target].r.down = true;
                curstates[target].l.down = false;
            } else {
                curstates[target].r.down = false;
                curstates[target].l.down = false;
            }
            if(curstates[target].y < -0.5) {
                curstates[target].u.down = false;
                curstates[target].d.down = true;
            } else if(curstates[target].y > 0.5) {
                curstates[target].u.down = true;
                curstates[target].d.down = false;
            } else {
                curstates[target].u.down = false;
                curstates[target].d.down = false;
            }
            for(int j = 0; j < curstates[target].keys.size(); j++) {
                curstates[target].keys[j].down = SDL_JoystickGetButton(joysticks[i], j);
            }
        }
        CHECK(controllers.size() == curstates.size());
        for(int i = 0; i < curstates.size(); i++) {
            controllers[i].newState(curstates[i]);
        }
		polling += bencher.ticksElapsed();
		bencher = Timer();
        if(interfaceRunTick( controllers ))
            quit = true;
		ticking += bencher.ticksElapsed();
		bencher = Timer();
		timer.waitForNextFrame();
		waiting += bencher.ticksElapsed();
		bencher = Timer();
		if( !timer.skipFrame()) {
			initFrame();
			interfaceRenderToScreen();
			deinitFrame();
		} else {
            skipped++;
		}
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
            dprintf( "%4d skipped", skipped );
            dprintf( "%f clusters/frame last 60 frames", getAccumulatedClusterCount() / float(60 - skipped));
			polling = 0;
			ticking = 0;
			waiting = 0;
			rendering = 0;
            skipped = 0;
		}
        frako++;
	}
    for(int i = 0; i < joysticks.size(); i++)
        SDL_JoystickClose(joysticks[i]);
}
