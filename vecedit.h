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

struct Selectitem {
  enum { PATHCENTER, NODE, CURVECONTROL, LINK, NONE };
  int type;
  int path;
  int item;
  bool curveside;
  
  Selectitem();
  Selectitem(int type, int path);
  Selectitem(int type, int path, int item);
  Selectitem(int type, int path, int item, bool curveside);
};

enum Cursor { CURSOR_NORMAL, CURSOR_CROSS, CURSOR_HAND, CURSOR_UNCHANGED = -1 };

struct OtherState {
  Cursor cursor;
  bool redraw;
  
  OtherState();
};

class Vecedit {
  bool modified;
  
  // center, zoom per pixel
  Float2 center;
  float zpp;
  
  Dvec2 dv2;
  
  enum { IDLE, SELECTED, SELECTEDNEW, DRAGGING };
  int state;
  Float2 startpos;
  Selectitem select;
  
  OtherState ostate;
  int grid;
  
  vector<Selectitem> getSelectionStack(Float2 pos) const;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres) const;
  void setScrollPos(Float2 scrollpos);

  OtherState mouse(const MouseInput &mouse);
  OtherState gridupd(int gridsize);

  void render() const;

  void clear();
  void load(const string &filename);
  bool save(const string &filename);

  void registerEmergencySave();
  void unregisterEmergencySave();

  explicit Vecedit();
};

#endif
