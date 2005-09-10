
#include "main.h"
#include "const.h"

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
#include "gfx.h"
#include "itemdb.h"

#define SCREEN_DEPTH  16
SDL_Surface * MainWindow = NULL;

DEFINE_bool(fullscreen, true, "Fullscreen");
DEFINE_bool(help, false, "Get help");

int GetVideoFlags( void ) {

	int videoflags = 0;

    videoflags = SDL_OPENGL | SDL_HWPALETTE/* | SDL_RESIZABLE*/;

    const SDL_VideoInfo *videoinfo = SDL_GetVideoInfo();
	CHECK( videoinfo );
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
    
    if(FLAGS_fullscreen)
        videoflags |= SDL_FULLSCREEN;

	return videoflags;

}

void MakeWindow( const char * strWindowName, int width, int height ) {

	dprintf( "Startmake\n" );

	CHECK( height > 0 );
	CHECK( width > 0 );

	dprintf( "Mainwindow\n" );

	// appears to be crashing here sometimes?
    MainWindow = SDL_SetVideoMode( width, height, SCREEN_DEPTH, GetVideoFlags() );
	CHECK( MainWindow );

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
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, SCREEN_DEPTH);         // size of depth buffer
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0);          // we aren't going to use the stencil buffer
    SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0);        // this and the next three lines set the bits allocated per pixel -
    SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0);      // - for the accumulation buffer to 0
    SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0);

}

void initSystem() {

    CHECK(SDL_Init( SDL_INIT_VIDEO ) >= 0);
    CHECK(SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0);

	SetupOgl();
    MakeWindow( "Devastation Net", SCREEN_WIDTH, SCREEN_HEIGHT );

};

void deinitSystem() {

	SDL_Quit();

};

int main( int argc, char **argv ) {

	dprintf( "Init\n" );
    
    initFlags(argc, argv);
    if(FLAGS_help) {
        map<string, string> flags = getFlagDescriptions();
        #undef printf
        for(map<string, string>::iterator itr = flags.begin(); itr != flags.end(); itr++)
            printf("%s: %s\n", itr->first.c_str(), itr->second.c_str());
        return 0;
    }
    
    initItemdb();

	initSystem();
    initGfx();

	dprintf( "Loop\n" );

	MainLoop();

	dprintf( "Deinit\n" );

	deinitSystem();

	dprintf( "Done\n" );
    
    return 0;
}
