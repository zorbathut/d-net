
#include "vecedit.h"

bool Vecedit::changed() const {
  return true;
}

Vecedit::Vecedit(const smart_ptr<Closure0> &resync_gui_callback) : resync_gui_callback(resync_gui_callback) { };
