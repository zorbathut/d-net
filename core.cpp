
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

Keystates curstate;

void keyPress( SDL_KeyboardEvent *key ) {
	char *ps = NULL;
	if( key->keysym.sym == SDLK_UP )
		ps = &curstate.forward;
	if( key->keysym.sym == SDLK_DOWN )
		ps = &curstate.back;
	if( key->keysym.sym == SDLK_LEFT )
		ps = &curstate.left;
	if( key->keysym.sym == SDLK_RIGHT )
		ps = &curstate.right;
	if( !ps )
		return;
	if( key->type == SDL_KEYUP )
		*ps = 0;
	if( key->type == SDL_KEYDOWN )
		*ps = 1;
}

void MainLoop() {
	memset( &curstate, 0, sizeof( curstate ) );

	Game game;
	Timer timer;

	bool quit = false;

	while( !quit ) {
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
		setZoom( 0, 0, 100 );
		vector< Keystates > ks;
		ks.push_back( curstate );
		game.runTick( ks );
		timer.waitForNextFrame();
		if( !timer.skipFrame() ) {
			initFrame();
			game.renderToScreen( RENDERTARGET_SPECTATOR );
			deinitFrame();
		} else {
			dprintf( "Skipped!\n" );
		}
		timer.frameDone();
	}
}
