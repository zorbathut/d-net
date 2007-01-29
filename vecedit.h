#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"
#include "dvec2.h"
#include "input.h"

struct MouseInput {
  Float2 pos;
  
  Button b[2];
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

struct WrapperState {
  Float2 center;
  float zpp;
  
  int grid;
  
  WrapperState();
};

struct OtherState {
  Cursor cursor;
  bool redraw;
  
  bool snapshot;
  
  OtherState();
};

class Vecedit {
  bool modified;
  
  Dvec2 dv2;
  
  enum { IDLE, SELECTED, SELECTEDNEW, DRAGGING };
  int state;
  Float2 startpos;
  Selectitem select;
  
  vector<Selectitem> getSelectionStack(Float2 pos, float zpp) const;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres, const WrapperState &state) const;

  OtherState mouse(const MouseInput &mouse, const WrapperState &state);

  void render(const WrapperState &state) const;

  void clear();
  void load(const string &filename);
  bool save(const string &filename);

  void registerEmergencySave();
  void unregisterEmergencySave();

  explicit Vecedit();
};

#endif
