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

enum Cursor { CURSOR_NORMAL, CURSOR_CROSS, CURSOR_HAND };

class Vecedit {
  smart_ptr<Closure<> > resync_gui_callback;
  smart_ptr<Closure<Cursor> > cursor_change_callback;
  
  // center, zoom per pixel
  Float2 center;
  float zpp;
  
  Dvec2 dv2;
  
  int selected_path;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres) const;
  void setScrollPos(Float2 scrollpos);

  void render() const;

  void mouse(const MouseInput &mouse);

  void clear();
  void load(const string &filename);
  bool save(const string &filename);
  
  explicit Vecedit(const smart_ptr<Closure<> > &resync_gui_callback, const smart_ptr<Closure<Cursor> > &cursor_change_callback);
};

#endif
