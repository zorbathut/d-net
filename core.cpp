
#include <iostream>
#include <assert.h>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "core.h"
#include "main.h"
#include "game.h"
#include "timer.h"
#include "debug.h"
#include "gfx.h"

vector< Keystates > curstates( 2 );

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
	if( key->keysym.sym == SDLK_w )
		ps = &curstates[1].forward;
	if( key->keysym.sym == SDLK_s )
		ps = &curstates[1].back;
	if( key->keysym.sym == SDLK_a )
		ps = &curstates[1].left;
	if( key->keysym.sym == SDLK_d )
		ps = &curstates[1].right;
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

	Game game;
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
					assert( 0 );
					//CreateWindow( "Destruction Net", event.resize.w, event.resize.h );
					break;

				default:
					break;
			}
		}
		polling += bencher.ticksElapsed();
		bencher = Timer();
		game.runTick( curstates );
		ticking += bencher.ticksElapsed();
		bencher = Timer();
		timer.waitForNextFrame();
		waiting += bencher.ticksElapsed();
		bencher = Timer();
		if( !timer.skipFrame() ) {
			setZoom( 0, 0, 100 );
			initFrame();
			game.renderToScreen( RENDERTARGET_SPECTATOR );
			deinitFrame();
		} else {
			dprintf( "Skipped!\n" );
		}
		rendering += bencher.ticksElapsed();
		timer.frameDone();
		frako++;
		if( frako % 60 == 0 ) {
			long long tot = polling + ticking + waiting + rendering;
			dprintf( "%4d polling", int( polling * 1000 / tot ) );
			dprintf( "%4d ticking", int( ticking * 1000 / tot ) );
			dprintf( "%4d waiting", int( waiting * 1000 / tot ) );
			dprintf( "%4d rendering", int( rendering * 1000 / tot ) );
		}
	}
}
