#ifndef DNET_VECEDIT
#define DNET_VECEDIT

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
private:
  void flatten();

public:
  enum Type { ENTITY, ENTITYROTATE, NODE, CURVECONTROL, PATHROTATE, PATHCENTER, LINK, END, NONE = -1};
  Type type;
  
  int path;
  int item;
  bool curveside;
  
  int entity;
  
  SelectItem();
  SelectItem(Type type, int x);
  SelectItem(Type type, int path, int item);
  SelectItem(Type type, int path, int item, bool curveside);
};

enum CursorMode { CURSORMODE_NORMAL, CURSORMODE_CROSS, CURSORMODE_HAND, CURSORMODE_UNCHANGED = -1 };

struct UIState {
  bool newPath;
  bool newNode;
  bool newTank;
  
  UIState();
};

struct WrapperState {
  Float2 center;
  float zpp;
  
  int grid;
  int rotgrid;
  
  bool showControls;
  
  UIState ui;
  
  WrapperState();
};

struct OtherState {
  UIState ui;
  CursorMode cursor;

  bool redraw;
  bool snapshot;
  
  bool hasPathProperties;
  int divisions;
  bool snowflakey;
  
  int mintanks;
  int maxtanks;
  
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
  
  void pathprops(OtherState *ost) const;
  
public:

  bool changed() const;
  ScrollBounds getScrollBounds(Float2 screenres, const WrapperState &state) const;

  OtherState mouse(const MouseInput &mouse, const WrapperState &state);
  OtherState del(const WrapperState &wrap);
  OtherState rotate(int reflects, const WrapperState &wrap);
  OtherState snowflake(bool newstate, const WrapperState &wrap);

  OtherState mintanks(int mp, const WrapperState &state);
  OtherState maxtanks(int mp, const WrapperState &state);

  void render(const WrapperState &state) const;

  void clear();
  void load(const string &filename);
  bool save(const string &filename);

  void registerEmergencySave();
  void unregisterEmergencySave();

  explicit Vecedit();
};

#endif
