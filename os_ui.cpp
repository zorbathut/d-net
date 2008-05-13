
#include "os_ui.h"

#include <windows.h> // :D

int Message(const string &text, bool yesno) {
  int rv = MessageBox(NULL, text.c_str(), "Devastation Net", yesno ? MB_OKCANCEL : MB_OK);
  if(rv == IDOK)
    return true;
  return false;
}
