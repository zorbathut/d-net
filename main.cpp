
#include "args.h"
#include "core.h"
#include "debug.h"
#include "gfx.h"
#include "itemdb.h"
#include "os.h"
#include "os_ui.h"
#include "httpd.h"
#include "generators.h"
#include "audio.h"
#include "test.h"
#include "res_interface.h"
#include "settings.h"
#include "init.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
  #include <SDL/SDL.h>
#else
  #include <GL/gl.h>
  #include <SDL.h>
#endif

#include <boost/assign.hpp>

using namespace std;

DEFINE_bool(help, false, "Get help");
DEFINE_float(generateCachedShops, -1, "Do all the work necessary to cache shops. Parameter is the accuracy");
//DEFINE_bool(generateWeaponStats, false, "Do all the work necessary to dump weapon info");
DEFINE_bool(generateFactionStats, false, "Do all the work necessary to dump faction info");
DEFINE_bool(dumpText, false, "Dump all \"text\" blocks");

DEFINE_bool(runTests, true, "Run all tests");
DEFINE_bool(runGame, true, "Run the actual game");

DEFINE_bool(outputLevelChart, false, "Output a chart of which levels are available for what playercounts, then quit");

DECLARE_bool(shopcache);
DECLARE_bool(render);

void SetupOgl() {

  CHECK(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) == 0);     // tell SDL that the GL drawing is going to be double buffered
  CHECK(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32) == 0);     // size of depth buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1) == 0);      // now we do use the stencil buffer
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0) == 0);    // this and the next three lines set the bits allocated per pixel -
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0) == 0);    // - for the accumulation buffer to 0
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0) == 0);
  CHECK(SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0) == 0);

}

void initSystem() {
  CHECK(SDL_Init(SDL_INIT_NOPARACHUTE) >= 0);
  CHECK(SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0);
  CHECK(SDL_InitSubSystem(SDL_INIT_TIMER) >= 0);
  CHECK(SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0);
}

void initVideo() {
  StackString sst("InitVideo");
  
  CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0);
  
  SetupOgl();
  if(!setResolution(make_pair(Settings::get_instance().res_x, Settings::get_instance().res_y), Settings::get_instance().res_aspect, Settings::get_instance().res_fullscreen)) {
    if(!setResolution(make_pair(640, 480), 4.0/3.0, false)) {
      Message("Can't find your 3d hardware, aborting.", false);
      CHECK(0);
    }
  }
  
  {
    StackString sst("GL minituae");
    dprintf("GL version: %s\n", glGetString(GL_VERSION));
 
    dprintf("Renderer: %s\n", glGetString(GL_RENDERER));
    dprintf("Vendor: %s\n", glGetString(GL_VENDOR));
    
    GLint v;
    glGetIntegerv(GL_DEPTH_BITS, &v);
    dprintf("Depth bits: %d\n", (int)v);
    glGetIntegerv(GL_STENCIL_BITS, &v);
    dprintf("Stencil bits: %d\n", (int)v);
    
    if(0) {
      typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
      char* extensions = (char*)glGetString(GL_EXTENSIONS);
      if(strstr(extensions, "WGL_EXT_swap_control")) {
        dprintf("Disabling vsync\n");
        PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
        wglSwapIntervalEXT = (PFNWGLEXTSWAPCONTROLPROC)SDL_GL_GetProcAddress("wglSwapIntervalEXT");
        wglSwapIntervalEXT(0);
      } else {
        dprintf("Can't disable vsync\n");
      }
    }
  }
  
  initGfx();
}

void deinitSystem() {

  SDL_Quit();

};

int main(int argc, char **argv) {
  setInitFlagFile("settings");
  initProgram(&argc, &argv);
  
  StackString sst("Main");
  stackStart = &sst;

  dprintf("Init\n");
  
  if(FLAGS_help) {
    map<string, string> flags = getFlagDescriptions();
    #undef printf
    for(map<string, string>::iterator itr = flags.begin(); itr != flags.end(); itr++)
      printf("%s: %s\n", itr->first.c_str(), itr->second.c_str());
    return 0;
  }
  if(FLAGS_fileroot.size() && FLAGS_fileroot[FLAGS_fileroot.size() - 1] != '/')
    FLAGS_fileroot += '/';
  
  if(FLAGS_generateCachedShops != -1) {
    FLAGS_shopcache = false;
  }
  
  Settings::get_instance().load();
  
  loadItemdb();
  
  {
    bool generated = false;
    
    if(FLAGS_generateCachedShops != -1) {
      CHECK(FLAGS_generateCachedShops > 0 && FLAGS_generateCachedShops < 1);
      generateCachedShops(FLAGS_generateCachedShops);
      generated = true;
    }
    
    /*if(FLAGS_generateWeaponStats) {
      generateWeaponStats();
      generated = true;
    }*/
    
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
  
  if(FLAGS_runTests)
    runTests();
  
  if(FLAGS_outputLevelChart)
    outputLevelChart();

  if(FLAGS_runGame) {
    StackString sst("RunGame");
    initHttpd();
    initSystem();
    if(FLAGS_render) {
      initVideo();
    } else {
      initWindowing(4./3.);
    }
    loadFonts();
    initAudio();
    MainLoop();
  }

  deinitSystem();
  
  deinitHttpd();
  
  Settings::get_instance().save();
  
  dprintf("Leaving main");
  
  return 0;
}
