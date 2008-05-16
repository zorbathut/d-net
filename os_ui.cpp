
#include "os_ui.h"

#ifndef NO_WINDOWS

#include <windows.h> // :D

int Message(const string &text, bool yesno) {
  int rv = MessageBox(NULL, text.c_str(), "Devastation Net", yesno ? MB_OKCANCEL : MB_OK);
  if(rv == IDOK)
    return true;
  return false;
}

pair<int, int> getScreenRes() {
  pair<int, int> glorb;
  glorb.first = GetSystemMetrics(SM_CXSCREEN);
  glorb.second = GetSystemMetrics(SM_CYSCREEN);
  return glorb;
}

#else

#include <gdk/gdk.h>

pair<int, int> getScreenRes() {
  GdkScreen *screen = gdk_screen_get_default();
  return make_pair(gdk_screen_get_width(screen), gdk_screen_get_height(screen));
}

#endif

