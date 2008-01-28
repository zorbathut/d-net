
#include "res_interface.h"

#include "debug.h"
#include "gfx.h"
#include "settings.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
  #include <SDL/SDL.h>
#else
  #include <GL/gl.h>
  #include <SDL.h>
#endif

int GetVideoFlags(bool fullscreen) {

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
  
  dprintf("Attempting to make window %d/%d\n", width, height);

  CHECK(height > 0);
  CHECK(width > 0);
  
  SDL_Surface *MainWindow = SDL_SetVideoMode(width, height, 32, GetVideoFlags(fullscreen));
  if(!MainWindow)
    return false;

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
  if(!MakeWindow("Devastation Net", res.first, res.second, fullscreen))
    return false;
  
  updateResolution(aspect);

  Settings::get_instance().res_x = res.first;
  Settings::get_instance().res_y = res.second;
  Settings::get_instance().res_aspect = aspect;
  Settings::get_instance().res_fullscreen = fullscreen;
  
  cres = res;
  caspect = aspect;
  cfull = fullscreen;

  return true;
}

vector<pair<int, int> > getResolutions() {
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
