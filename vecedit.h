#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"
#include "dvec2.h"
#include "input.h"

struct MouseInput {
  Float2 pos;
  
  int dw;
  
  Button b[3];
};

struct ScrollBounds {
  Float4 objbounds;
  Float4 currentwindow;
};

class Vecedit {
  smart_ptr<Closure<> > resync_gui_callback;
  
  // center, zoom per pixel
  Float2 center;
  float zpp;
  
  Dvec2 dv2;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres) const;
  void setScrollPos(Float2 scrollpos);

  void render() const;

  void mouse(const MouseInput &mouse);

  void clear();
  void load(const string &filename);
  bool save(const string &filename);
  
  explicit Vecedit(const smart_ptr<Closure<> > &resync_gui_callback);
};

#endif
