
#include "res_interface.h"

#include "debug.h"
#include "gfx.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
  #include <SDL/SDL.h>
#else
  #include <GL/gl.h>
  #include <SDL.h>
#endif

int GetVideoFlags(bool fullscreen) {

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

bool setResolution(int x, int y, float aspect, bool fullscreen) {
  if(!MakeWindow("Devastation Net", x, y, fullscreen))
    return false;
  
  updateResolution(aspect);

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


/*
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
*/
