
#include "args.h"
#include "core.h"
#include "debug.h"
#include "gfx.h"
#include "itemdb.h"
#include "os.h"
#include "httpd.h"
#include "generators.h"
#include "audio.h"
#include "test.h"


#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
  #include <SDL/SDL.h>
#else
  #include <GL/gl.h>
  #include <SDL.h>
#endif

#include <boost/assign.hpp>

using namespace std;

#define SCREEN_DEPTH  16
SDL_Surface * MainWindow = NULL;

DEFINE_bool(fullscreen, true, "Fullscreen");
DEFINE_bool(help, false, "Get help");
DEFINE_float(generateCachedShops, -1, "Do all the work necessary to cache shops. Parameter is the accuracy");
DEFINE_bool(generateWeaponStats, false, "Do all the work necessary to dump weapon info");
DEFINE_bool(generateFactionStats, false, "Do all the work necessary to dump faction info");
DEFINE_bool(dumpText, false, "Dump all \"text\" blocks");

DEFINE_bool(runTests, true, "Run all tests");
DEFINE_bool(runGame, true, "Run the actual game");

DECLARE_bool(shopcache);

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
  CHECK(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SCREEN_DEPTH) == 0);     // size of depth buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1) == 0);      // now we do use the stencil buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0) == 0);    // this and the next three lines set the bits allocated per pixel -
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0) == 0);    // - for the accumulation buffer to 0
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0) == 0);

}

DEFINE_int(resolution_x, -1, "X resolution (Y is X/4*3), -1 for autodetect");

void resDown() {
  const int reses[] = { 1600, 1400, 1280, 1152, 1024, 800, 640, -1 };
  CHECK(FLAGS_resolution_x > 0);
  for(int i = 0; i < ARRAY_SIZE(reses); i++) {
    if(FLAGS_resolution_x > reses[i]) {
      FLAGS_resolution_x = reses[i];
      break;
    }
  }
  CHECK(FLAGS_resolution_x > 0);
}

void setDefaultResolution(bool fullscreen) {
  const SDL_VideoInfo *vinf = SDL_GetVideoInfo();
  
  dprintf("Current detected resolution: %d/%d\n", vinf->current_w, vinf->current_h);
  if(FLAGS_resolution_x == -1) {
    FLAGS_resolution_x = vinf->current_w;
    if(!fullscreen)
      resDown();
  }
}
  
int getFlagResX() {
  CHECK(FLAGS_resolution_x > 0);
  CHECK(FLAGS_resolution_x % 4 == 0);
  return FLAGS_resolution_x;
}
int getFlagResY() {
  CHECK(FLAGS_resolution_x > 0);
  CHECK(FLAGS_resolution_x % 4 == 0);
  return FLAGS_resolution_x / 4 * 3;
}

void initSystem() {

  dprintf("inp\n");
  CHECK(SDL_Init(SDL_INIT_NOPARACHUTE) >= 0);
  dprintf("iv\n");
  CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0);
  dprintf("ia ia cthulhu fhtagn\n");
  CHECK(SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0);
  dprintf("it\n");
  CHECK(SDL_InitSubSystem(SDL_INIT_TIMER) >= 0);
  dprintf("ij\n");
  CHECK(SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0);
  dprintf("ogl\n");

  SetupOgl();
  setDefaultResolution(FLAGS_fullscreen);
  while(!MakeWindow("Devastation Net", getFlagResX(), getFlagResY()))
    resDown();
  
  {
    dprintf("GL version: %s\n", glGetString(GL_VERSION));
 
    dprintf("Renderer: %s\n", glGetString(GL_RENDERER));
    dprintf("Vendor: %s\n", glGetString(GL_VENDOR));
    
    GLint v;
    glGetIntegerv(GL_DEPTH_BITS, &v);
    dprintf("Depth bits: %d\n", (int)v);
    glGetIntegerv(GL_STENCIL_BITS, &v);
    dprintf("Stencil bits: %d\n", (int)v);
  }
};

void deinitSystem() {

  SDL_Quit();

};

int main(int argc, char **argv) {
  StackString sst("Main");
  stackStart = &sst;
  
  set_exename(argv[0]);

  dprintf("Init\n");
  
  initFlags(argc, argv, 0);
  if(FLAGS_help) {
    map<string, string> flags = getFlagDescriptions();
    #undef printf
    for(map<string, string>::iterator itr = flags.begin(); itr != flags.end(); itr++)
      printf("%s: %s\n", itr->first.c_str(), itr->second.c_str());
    return 0;
  }
  
  if(FLAGS_generateCachedShops != -1) {
    FLAGS_shopcache = false;
  }
  
  initItemdb();
  
  {
    bool generated = false;
    
    if(FLAGS_generateCachedShops != -1) {
      CHECK(FLAGS_generateCachedShops > 0 && FLAGS_generateCachedShops < 1);
      generateCachedShops(FLAGS_generateCachedShops);
      generated = true;
    }
    
    if(FLAGS_generateWeaponStats) {
      generateWeaponStats();
      generated = true;
    }
    
    if(FLAGS_generateFactionStats) {
      generateFactionStats();
      generated = true;
    }
    
    if(FLAGS_dumpText) {
      dumpText();
      generated = true;
    }
    
    if(generated)
      return 0;
  }
  
  initHttpd();

  initSystem();
  initGfx();
  updateResolution(4.0 / 3.0);
  initAudio();
  
  if(FLAGS_runTests)
    runTests();

  if(FLAGS_runGame)
    MainLoop();

  deinitSystem();
  
  deinitHttpd();
  
  dprintf("Leaving main");
  
  return 0;
}
