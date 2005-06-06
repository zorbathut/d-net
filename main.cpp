
#include "main.h"

#include <iostream>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "core.h"
#include "debug.h"
#include "util.h"
#include "args.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_DEPTH  16
SDL_Surface * MainWindow = NULL;

int GetVideoFlags( void ) {

	int videoflags = 0;

    videoflags = SDL_OPENGL | SDL_HWPALETTE/* | SDL_RESIZABLE*/;

    const SDL_VideoInfo *videoinfo = SDL_GetVideoInfo();
	assert( videoinfo );
    if( videoinfo->hw_available )
        videoflags |= SDL_HWSURFACE;
	else {
		dprintf( "WARNING: Software surface\n" );
        videoflags |= SDL_SWSURFACE;
	}
    
	if( videoinfo->blit_hw ) {
        videoflags |= SDL_HWACCEL;
	} else {
		dprintf( "WARNING: Software blit\n" );
	}

	return videoflags;

}

void MakeWindow( const char * strWindowName, int width, int height ) {

	dprintf( "Startmake\n" );

	assert( height > 0 );
	assert( width > 0 );

	dprintf( "Mainwindow\n" );

	// appears to be crashing here sometimes?
    MainWindow = SDL_SetVideoMode( width, height, SCREEN_DEPTH, GetVideoFlags() );
	assert( MainWindow );

	dprintf( "Caption\n" );

    SDL_WM_SetCaption( strWindowName, strWindowName );       // set the window caption (first argument) and icon caption (2nd arg)

	dprintf( "Viewport\n" );

    glViewport( 0, 0, width, height );

	dprintf( "Ident/ortho\n" );

    glLoadIdentity();
	glOrtho( 0, 1.25, 0, 1, 1.0, -1.0 );	

	dprintf( "Donemake\n" );

}

void DestroyWindow() {
	// does this need to do anything?
}

void SetupOgl() {

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );         // tell SDL that the GL drawing is going to be double buffered
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE,   SCREEN_DEPTH);         // size of depth buffer
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0);          // we aren't going to use the stencil buffer
    SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0);        // this and the next three lines set the bits allocated per pixel -
    SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0);      // - for the accumulation buffer to 0
    SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0);

}

void initSystem() {

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		assert( 0 );

	SetupOgl();
    MakeWindow( "Devastation Net", SCREEN_WIDTH, SCREEN_HEIGHT );

};

void deinitSystem() {

	SDL_Quit();

};

int main( int argc, char **argv ) {

	dprintf( "Init\n" );
    
    initFlags(argc, argv);

	initSystem();

	dprintf( "Loop\n" );

	MainLoop();

	dprintf( "Deinit\n" );

	deinitSystem();

	dprintf( "Done\n" );

}

void toggleFullscreen( void ) {
    if( SDL_WM_ToggleFullScreen( MainWindow ) == 0 )
		assert( 0 );
}


