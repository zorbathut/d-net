
#include <iostream>
#include <assert.h>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "core.h"
#include "main.h"
#include "game.h"
#include "timer.h"

void RenderScene() {

	glClearColor( 0.25f, 0.25f, 0.25f, 0.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0f, 0.0f, 0.0f);
	glRectf(0.1, 0.2, 0.2, 0.1);
	glFlush();
	SDL_GL_SwapBuffers(); 

}

void keyPress( SDL_keysym* key ) {
    switch( key->sym ) {
        case SDLK_F1:
            toggleFullscreen();
            break;

		default:
            break;
    }
}

void MainLoop() {

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
					keyPress( & event. key.keysym );         // callback for handling keystrokes, arg is key pressed
					break;

				case SDL_VIDEORESIZE:
					assert( 0 );
					//CreateWindow( "Destruction Net", event.resize.w, event.resize.h );
					break;

				default:
					break;
			}
		}
		game.runTick( vector< Keystates >() );
		timer.waitForNextFrame();
		if( !timer.skipFrame() ) {
			RenderScene();
			game.renderToScreen( RENDERTARGET_SPECTATOR );
		}
		timer.frameDone();
	}
}
