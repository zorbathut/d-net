
#include "vecedit.h"

bool Vecedit::changed() const {
  return true;
}

void Vecedit::clear() {
  dprintf("Clearing\n");
}
void Vecedit::load(const string &filename) {
  dprintf("Loading %s\n", filename.c_str());
}
void Vecedit::save(const string &filename) {
  dprintf("Saving %s\n", filename.c_str());
}

Vecedit::Vecedit(const smart_ptr<Closure0> &resync_gui_callback) : resync_gui_callback(resync_gui_callback) { };
