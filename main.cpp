
#include "main.h"

#include <iostream>
#include <assert.h>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "core.h"

void Init();
void MainLoop(void);

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
    else
        videoflags |= SDL_SWSURFACE;
    
    if( videoinfo->blit_hw )
        videoflags |= SDL_HWACCEL;

	return videoflags;

}

void CreateWindow( const char * strWindowName, int width, int height ) {

	assert( height > 0 );
	assert( width > 0 );

    MainWindow = SDL_SetVideoMode( width, height, SCREEN_DEPTH, GetVideoFlags() );
	assert( MainWindow );

    SDL_WM_SetCaption( strWindowName, strWindowName );       // set the window caption (first argument) and icon caption (2nd arg)

    glViewport( 0, 0, width, height );

    glLoadIdentity();
	glOrtho( 0, 1.25, 0, 1, 1.0, -1.0 );	

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
    CreateWindow( "Destruction Net", SCREEN_WIDTH, SCREEN_HEIGHT );

};

void deinitSystem() {

	SDL_Quit();

};

int main( int argc, char **argv ) {

	initSystem();

	MainLoop();

	deinitSystem();                                     

}

void toggleFullscreen(void)
{
    if( SDL_WM_ToggleFullScreen( MainWindow ) == 0 )
		assert( 0 );
}


