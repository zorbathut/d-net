
#include <iostream>
#include <assert.h>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

void Init();
void MainLoop(void);

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_DEPTH  16
SDL_Surface * MainWindow = NULL;

int GetVideoFlags( void ) {

	int videoflags = 0;

    videoflags = SDL_OPENGL | SDL_HWPALETTE | SDL_RESIZABLE;

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

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

	gluPerspective( 45.0f, (GLfloat)width / height, 1, 200.0f );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

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

// this stuff gets ripped out
void RenderScene() 
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    // Clear The Screen And The Depth Buffer
    glLoadIdentity();                                      // Reset The View
    
      //     Position      View     Up Vector
    gluLookAt(0, 0, 6,     0, 0, 0,     0, 1, 0);    // This determines where the camera's position and view is

    // The position has an X Y and Z.  Right now, we are standing at (0, 0, 6)
    // The view also has an X Y and Z.  We are looking at the center of the axis (0, 0, 0)
    // The up vector is 3D too, so it has an X Y and Z.  We say that up is (0, 1, 0)
    // Unless you are making a game like Descent(TM), the up vector can stay the same.

                              // Below we say that we want to draw triangles    
    glBegin (GL_TRIANGLES);                // This is our BEGIN to draw
            glVertex3f(0, 1, 0);        // Here is the top point of the triangle
      glVertex3f(-1, 0, 0);  glVertex3f(1, 0, 0);  // Here are the left and right points of the triangle
    glEnd();                      // This is the END of drawing

    // I arranged the functions like that in code so you could visualize better
    // where they will be on the screen.  Usually they would each be on their own line
    // The code above draws a triangle to those points and fills it in.
    // You can have as many points inside the BEGIN and END, but it must be in three's.
    // Try GL_LINES or GL_QUADS.  Lines are done in 2's and Quads done in 4's.

    SDL_GL_SwapBuffers();                                  // Swap the backbuffers to the foreground
}

void ToggleFullScreen(void)
{
    if(SDL_WM_ToggleFullScreen(MainWindow) == 0)           // try to toggle fullscreen mode for window 'MainWindow'
		assert( 0 );
}

void HandleKeyPressEvent(SDL_keysym * keysym)
{
    switch(keysym -> sym)                                  // which key have we got
    {
        case SDLK_F1 :                                     // if it is F1
            ToggleFullScreen();                            // toggle between fullscreen and windowed mode
            break;

        //case SDLK_ESCAPE:                                  // if it is ESCAPE
          //  Quit(0);                                       // quit after cleaning up
            
        default:                                           // any other key
            break;                                         // nothing to do
    }
}

void MainLoop(void)
{
    bool quit = false;                                     // is our job done ? not yet !
    SDL_Event event;

    while(! quit)                                          // as long as our job's not done
    {
        while( SDL_PollEvent(& event) )                    // look for events (like keystrokes, resizing etc.)
        {
            switch ( event.type )                          // what kind of event have we got ?
            {
			case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_KEYDOWN:
                    //HandleKeyPressEvent( & event. key.keysym );         // callback for handling keystrokes, arg is key pressed
                    break;

				case SDL_VIDEORESIZE:
                    CreateWindow( "Destruction Net", event.resize.w, event.resize.h );
                    break;

                default:                                   // any other event
                    break;                                 // nothing to do
            } // switch
        } // while( SDL_ ...

        RenderScene();                                     // draw our OpenGL scene
    } // while( ! done)
}
