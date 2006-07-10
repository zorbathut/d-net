
#include "args.h"
#include "core.h"
#include "debug.h"
#include "gfx.h"
#include "itemdb.h"
#include "os.h"

#include <GL/glu.h>
#include <SDL.h>

using namespace std;

#define SCREEN_DEPTH  16
SDL_Surface * MainWindow = NULL;

DEFINE_bool(fullscreen, true, "Fullscreen");
DEFINE_bool(help, false, "Get help");

int GetVideoFlags(void) {

  int videoflags = 0;

  videoflags = SDL_OPENGL | SDL_HWPALETTE/* | SDL_RESIZABLE*/;

  const SDL_VideoInfo *videoinfo = SDL_GetVideoInfo();
  CHECK(videoinfo);
  if(videoinfo->hw_available)
    videoflags |= SDL_HWSURFACE;
  else {
    dprintf("WARNING: Software surface\n");
    videoflags |= SDL_SWSURFACE;
  }
  
  if(videoinfo->blit_hw) {
    videoflags |= SDL_HWACCEL;
  } else {
    dprintf("WARNING: Software blit\n");
  }
  
  if(FLAGS_fullscreen)
    videoflags |= SDL_FULLSCREEN;

  return videoflags;

}

bool MakeWindow(const char * strWindowName, int width, int height) {
  
  dprintf("Attempting to make window %d/%d\n", width, height);

  CHECK(height > 0);
  CHECK(width > 0);
  
  MainWindow = SDL_SetVideoMode(width, height, SCREEN_DEPTH, GetVideoFlags());
  if(!MainWindow)
    return false;

  SDL_WM_SetCaption(strWindowName, strWindowName);     // set the window caption (first argument) and icon caption (2nd arg)

  glViewport(0, 0, width, height);

  glLoadIdentity();
  glOrtho(0, 1.25, 0, 1, 1.0, -1.0);  
  
  return true;

}

void SetupOgl() {

  CHECK(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) == 0);     // tell SDL that the GL drawing is going to be double buffered
  CHECK(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SCREEN_DEPTH) == 0);     // size of depth buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0) == 0);      // we aren't going to use the stencil buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0) == 0);    // this and the next three lines set the bits allocated per pixel -
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0) == 0);    // - for the accumulation buffer to 0
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0) == 0);

}

void GetStartingResolution() {
  setDefaultResolution(getCurrentScreenSize().first, getCurrentScreenSize().second, FLAGS_fullscreen);
}

void initSystem() {

  CHECK(SDL_Init(SDL_INIT_VIDEO) >= 0);
  CHECK(SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0);

  SetupOgl();
  GetStartingResolution();
  while(!MakeWindow("Devastation Net", getResolutionX(), getResolutionY()))
    resDown();
};

void deinitSystem() {

  SDL_Quit();

};

int main(int argc, char **argv) {
  
  StackString sst("Main");

  dprintf("Init\n");
  
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

  dprintf("Loop\n");

  MainLoop();

  dprintf("Deinit\n");

  deinitSystem();

  dprintf("Done\n");
  
  return 0;
}
