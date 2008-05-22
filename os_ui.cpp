
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

#include "init.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

int Message(const string &text, bool yesno) {
  GtkWidget *gd = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER, yesno ? GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK, text.c_str());
  int rv = gtk_dialog_run(GTK_DIALOG(gd));
  gtk_widget_destroy(gd);
  if(rv == GTK_RESPONSE_ACCEPT)
    return true;
  return false;
}

void gdk_init_shell(int *argc, const char ***argv) {
  gdk_init(argc, const_cast<char ***>(argv)); // rrrrgh
}
ADD_INITTER(gdk_init_shell, -10);

pair<int, int> getScreenRes() {
  GdkScreen *screen = gdk_screen_get_default();
  return make_pair(gdk_screen_get_width(screen), gdk_screen_get_height(screen));
}

#endif

