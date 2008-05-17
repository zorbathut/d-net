
#include "res_interface.h"

#include "debug.h"
#include "gfx.h"
#include "settings.h"
#include "args.h"
#include "os.h"
#include "os_ui.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
  #include <SDL/SDL.h>
#else
  #include <GL/gl.h>
  #include <SDL.h>
#endif

DECLARE_bool(render);

int GetVideoFlags(bool fullscreen) {
  StackString sst("GetVidFlags");

  int videoflags = 0;

  videoflags = SDL_OPENGL | SDL_HWPALETTE | SDL_RESIZABLE;

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
  
  if(fullscreen)
    videoflags |= SDL_FULLSCREEN;

  return videoflags;

}

bool MakeWindow(const char * strWindowName, int width, int height, bool fullscreen) {
  StackString sst("MakeWindow");
  
  dprintf("Attempting to make window %d/%d\n", width, height);

  CHECK(height > 0);
  CHECK(width > 0);
  
  SDL_Surface *MainWindow;
  {
    StackString sst("SDL apparently crashes sometimes! Amazing! Is that not amazing!");
    MainWindow = SDL_SetVideoMode(width, height, 32, GetVideoFlags(fullscreen));
  }
  
  if(!MainWindow) {
    dprintf("We has failed to make a window.");
    dprintf("%s\n", SDL_GetError());
    return false;
  }

  SDL_WM_SetCaption(strWindowName, strWindowName);     // set the window caption (first argument) and icon caption (2nd arg)

  glViewport(0, 0, width, height);

  glLoadIdentity();
  glOrtho(0, 1.25, 0, 1, 1.0, -1.0);  
  
  return true;
}

static pair<int, int> cres;
static float caspect;
static bool cfull;

bool setResolution(pair<int, int> res, float aspect, bool fullscreen) {
  StackString sst("SetRes");
  CHECK(FLAGS_render);
  
  if(res.first > getScreenRes().first || res.second > getScreenRes().second) {
    res = getScreenRes();
    aspect = (float)res.first / res.second;
  }
  
  dprintf("%dx%d, %f, %d\n", res.first, res.second, aspect, fullscreen);
  if(!MakeWindow("Devastation Net", res.first, res.second, fullscreen))
    return false;
  dprintf("We has made a window.");
  
  updateResolution(aspect);

  Settings::get_instance().res_x = res.first;
  Settings::get_instance().res_y = res.second;
  Settings::get_instance().res_aspect = aspect;
  Settings::get_instance().res_fullscreen = fullscreen;
  
  cres = res;
  caspect = aspect;
  cfull = fullscreen;
  
  SDL_ShowCursor(!fullscreen);

  return true;
}

vector<pair<int, int> > getResolutions() {
  StackString sst("GetReses");
  if(!FLAGS_render) {
    // Make some shit up
    return vector<pair<int, int> >(1, pair<int, int>(640, 480));
  }
  
  SDL_Rect **vmodes = SDL_ListModes(NULL, GetVideoFlags(true));
  
  CHECK(vmodes);
  CHECK(vmodes != (SDL_Rect**)-1);
  
  vector<pair<int, int> > rv;
  for(int i = 0; vmodes[i]; i++) {
    dprintf("%d x %d\n", vmodes[i]->w, vmodes[i]->h);
    rv.push_back(make_pair(vmodes[i]->w, vmodes[i]->h));
  }
  sort(rv.begin(), rv.end());
  
  return rv;
}

pair<int, int> getCurrentResolution() {
  return cres;
}
float getCurrentAspect() {
  return caspect;
}
bool getCurrentFullscreen() {
  return cfull;
}
