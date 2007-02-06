#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"
#include "dvec2.h"
#include "input.h"

#include <set>

using namespace std;

struct MouseInput {
  Float2 pos;
  
  Button b[2];
};

struct ScrollBounds {
  Float4 objbounds;
  Float4 currentwindow;
};

struct SelectItem {
  enum Type { PATHCENTER, NODE, CURVECONTROL, LINK, END, NONE = -1};
  Type type;
  int path;
  int item;
  bool curveside;
  
  SelectItem();
  SelectItem(Type type, int path);
  SelectItem(Type type, int path, int item);
  SelectItem(Type type, int path, int item, bool curveside);
};

enum Cursor { CURSOR_NORMAL, CURSOR_CROSS, CURSOR_HAND, CURSOR_UNCHANGED = -1 };

struct UIState {
  bool newPath;
  bool newNode;
  
  UIState();
};

struct WrapperState {
  Float2 center;
  float zpp;
  
  int grid;
  
  UIState ui;
  
  WrapperState();
};

struct OtherState {
  UIState ui;
  Cursor cursor;

  bool redraw;
  bool snapshot;
  
  bool hasPathProperties;
  int divisions;
  
  OtherState();
};

class SelectStack {
  SelectItem next;
  
  vector<SelectItem> items;
  
  vector<SelectItem> iteorder;
  
  set<SelectItem> possible_items;
  
public:
  bool hasItem(const SelectItem &si) const;
  bool hasItems() const;

  SelectItem nextItem() const;

  bool hasItemType(SelectItem::Type type) const;
  SelectItem getItemType(SelectItem::Type type) const;

  const vector<SelectItem> &itemOrder() const;

  SelectStack();
  SelectStack(const vector<SelectItem> &items, const SelectItem &current);
};

class Vecedit {
  bool modified;
  
  Dvec2 dv2;
  
  enum { IDLE, SELECTED, SELECTEDNOCHANGE, DRAGGING };
  int state;
  SelectItem select;
  
  Float2 startpos;
  SelectStack startposstack;
  
  SelectStack getSelectionStack(Float2 pos, float zpp) const;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres, const WrapperState &state) const;

  OtherState mouse(const MouseInput &mouse, const WrapperState &state);
  OtherState del(const WrapperState &wrap);

  void render(const WrapperState &state) const;

  void clear();
  void load(const string &filename);
  bool save(const string &filename);

  void registerEmergencySave();
  void unregisterEmergencySave();

  explicit Vecedit();
};

#endif
