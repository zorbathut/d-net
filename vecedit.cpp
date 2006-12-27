
#include "vecedit.h"
#include "gfx.h"

bool Vecedit::changed() const {
  return false;
}

void Vecedit::render() const {
  setZoomCenter(xc, yc, zpp * getResolutionY() / 2);
  setColor(Color(1.0, 1.0, 1.0));
  drawJustifiedText("THIS IS A TEST", 10, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
}

void Vecedit::clear() {
  dprintf("Clearing\n");
  resync_gui_callback->Run();
}
void Vecedit::load(const string &filename) {
  dprintf("Loading %s\n", filename.c_str());
  resync_gui_callback->Run();
}
void Vecedit::save(const string &filename) {
  dprintf("Saving %s\n", filename.c_str());
}

Vecedit::Vecedit(const smart_ptr<Closure0> &resync_gui_callback) : resync_gui_callback(resync_gui_callback) {
  xc = 0;
  yc = 0;
  zpp = 0.25;
};
