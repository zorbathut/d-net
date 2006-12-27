
#include "vecedit.h"
#include "gfx.h"

bool Vecedit::changed() const {
  return false;
}

void Vecedit::render() const {
  setZoomCenter(xc, yc, zpp * getResolutionY() / 2);
  setColor(Color(1.0, 1.0, 1.0));

  for(int i = 0; i < dv2.paths.size(); i++)
    drawVectorPath(dv2.paths[i], make_pair(Float2(0, 0), 0), 100, zpp * 2);
  
  setColor(0.5, 0.5, 0.5);
  drawGrid(32, zpp * 2);
}

void Vecedit::clear() {
  dv2 = Dvec2();
  resync_gui_callback->Run();
}
void Vecedit::load(const string &filename) {
  dv2 = loadDvec2(filename);
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
