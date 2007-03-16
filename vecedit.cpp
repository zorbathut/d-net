
#include "vecedit.h"

#include "gfx.h"
#include "util.h"

#include "itemdb.h"

#include <set>

using namespace std;

const float primenode = 8;
const float secondnode = 5;

const float rotatehandle = 40;

void SelectItem::flatten() {
  type = NONE;
  
  path = -1;
  item = -1;
  curveside = false;
  
  entity = -1;
}

SelectItem::SelectItem() {
  flatten();
};
SelectItem::SelectItem(Type in_type, int in_x) {
  flatten();
  
  if(in_type == PATHCENTER || in_type == PATHROTATE) {
    type = in_type;
    path = in_x;
  } else if(in_type == ENTITY || in_type == ENTITYROTATE) {
    type = in_type;
    entity = in_x;
  } else {
    CHECK(0);
  }
};
SelectItem::SelectItem(Type in_type, int in_path, int in_item) {
  CHECK(in_type == NODE || in_type == LINK);
  flatten();
  type = in_type;
  path = in_path;
  item = in_item;
};
SelectItem::SelectItem(Type in_type, int in_path, int in_item, bool in_curveside) {
  CHECK(in_type == CURVECONTROL);
  flatten();
  type = in_type;
  path = in_path;
  item = in_item;
  curveside = in_curveside;
};

bool operator<(const SelectItem &lhs, const SelectItem &rhs) {
  if(lhs.type != rhs.type) return lhs.type < rhs.type;
  if(lhs.path != rhs.path) return lhs.path < rhs.path;
  if(lhs.item != rhs.item) return lhs.item < rhs.item;
  if(lhs.curveside != rhs.curveside) return lhs.curveside < rhs.curveside;
  if(lhs.entity != rhs.entity) return lhs.entity < rhs.entity;
  return false;
}

bool operator==(const SelectItem &lhs, const SelectItem &rhs) {
  if(lhs.type != rhs.type) return false;
  if(lhs.path != rhs.path) return false;
  if(lhs.item != rhs.item) return false;
  if(lhs.curveside != rhs.curveside) return false;
  if(lhs.entity != rhs.entity) return false;
  return true;
}

bool isDraggable(int type) {
  return type == SelectItem::PATHCENTER || type == SelectItem::PATHROTATE || type == SelectItem::NODE || type == SelectItem::CURVECONTROL || type == SelectItem::ENTITY || type == SelectItem::ENTITYROTATE;
}

UIState::UIState() {
  newPath = false;
  newNode = false;
}

WrapperState::WrapperState() {
  center = Float2(0, 0);
  zpp = 1;
  grid = -1;
}

OtherState::OtherState() {
  cursor = CURSOR_UNCHANGED;
  redraw = false;
  snapshot = false;
}

bool SelectStack::hasItem(const SelectItem &si) const {
  return possible_items.count(si);
}
bool SelectStack::hasItems() const {
  return next.type != SelectItem::NONE;
}

SelectItem SelectStack::nextItem() const {
  return next;
}

bool SelectStack::hasItemType(SelectItem::Type type) const {
  return items[type].type != SelectItem::NONE;
}
SelectItem SelectStack::getItemType(SelectItem::Type type) const {
  return items[type];
}
const vector<SelectItem> &SelectStack::itemOrder() const {
  return iteorder;
}

SelectStack::SelectStack() { }; // This isn't really a valid item, because items.size() isn't correct.
SelectStack::SelectStack(const vector<SelectItem> &in_items, const SelectItem &current) {
  possible_items = set<SelectItem>(in_items.begin(), in_items.end());
  CHECK(possible_items.size() == in_items.size());
  
  items.resize(SelectItem::END);
  
  int spt;
  if(count(in_items.begin(), in_items.end(), current)) {
    spt = find(in_items.begin(), in_items.end(), current) - in_items.begin();
    next = in_items[modurot(spt + 1, in_items.size())];
  } else if(in_items.size()) {
    spt = 0;
    next = in_items[spt];
  }
  
  for(int i = 0; i < in_items.size(); i++) {
    if(items[in_items[modurot(spt + i, in_items.size())].type].type == SelectItem::NONE) {
      items[in_items[modurot(spt + i, in_items.size())].type] = in_items[modurot(spt + i, in_items.size())];
      iteorder.push_back(in_items[modurot(spt + i, in_items.size())]);
    }
  }
  
  for(int i = 0; i < items.size(); i++)
    CHECK(items[i].type == SelectItem::NONE || items[i].type == i);
};

void maybeAddPoint(vector<SelectItem> *ite, SelectItem tite, Float2 pos, Float2 loc, float dist) {
  if(max(abs(pos.x - loc.x), abs(pos.y - loc.y)) <= dist)
    ite->push_back(tite);
}

void maybeAddLine(vector<SelectItem> *ite, SelectItem tite, Float2 pos, Float4 loc, float dist) {
  if(linepointdistance(loc, pos) <= dist)
    ite->push_back(tite);
}

void maybeAddCurve(vector<SelectItem> *ite, SelectItem tite, Float2 pos, Float4 ptah, Float4 ptbh, float dist) {
  vector<Float2> curv = generateCurve(ptah, ptbh, 20);
  for(int i = 0; i < curv.size() - 1; i++) {
    if(linepointdistance(Float4(curv[i], curv[i + 1]), pos) <= dist) {
      ite->push_back(tite);
      break;
    }
  }
}

SelectStack Vecedit::getSelectionStack(Float2 pos, float zpp) const {
  vector<SelectItem> ites;
  
  for(int i = 0; i < dv2.paths.size(); i++) {
    const VectorPath &vp = dv2.paths[i];
    const bool thisselect = (i == select.path);
    if(thisselect) {
      maybeAddPoint(&ites, SelectItem(SelectItem::PATHCENTER, i), pos, vp.center, primenode * zpp * 1.5);
      if(vp.reflect)
        maybeAddPoint(&ites, SelectItem(SelectItem::PATHROTATE, i), pos, vp.center + makeAngle(2 * PI * vp.ang_numer / vp.ang_denom) * zpp * rotatehandle, secondnode * zpp * 1.5);
    }
    
    for(int j = 0; j < vp.vpath.size(); j++) {
      float selsize;
      if(thisselect) {
        selsize = primenode * zpp;
      } else {
        selsize = secondnode * zpp;
      }
      
      maybeAddPoint(&ites, SelectItem(SelectItem::NODE, i, j), pos, vp.center + vp.vpath[j].pos, selsize * 1.5);
      
      if(thisselect && vp.vpath[j].curvl) {
        maybeAddPoint(&ites, SelectItem(SelectItem::CURVECONTROL, i, j, false), pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvlp, secondnode * zpp * 1.5);
      }
      if(thisselect && vp.vpath[j].curvr) {
        maybeAddPoint(&ites, SelectItem(SelectItem::CURVECONTROL, i, j, true), pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvrp, secondnode * zpp * 1.5);
      }
      
      int k = (j + 1) % vp.vpath.size();
      if(vp.vpath[j].curvr) {
        CHECK(vp.vpath[k].curvl);
        maybeAddCurve(&ites, SelectItem(SelectItem::LINK, i, j), pos, Float4(vp.center + vp.vpath[j].pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvrp), Float4(vp.center + vp.vpath[k].pos + vp.vpath[k].curvlp, vp.center + vp.vpath[k].pos), selsize);
      } else {
        maybeAddLine(&ites, SelectItem(SelectItem::LINK, i, j), pos, Float4(vp.center + vp.vpath[j].pos, vp.center + vp.vpath[k].pos), selsize);
      }
    }
  }
  
  for(int i = 0; i < dv2.entities.size(); i++) {
    const Entity &ent = dv2.entities[i];
    
    if(ent.type == ENTITY_TANKSTART) {
      maybeAddPoint(&ites, SelectItem(SelectItem::ENTITY, i), pos, ent.pos, primenode * zpp);
      if(select.entity == i)
        maybeAddPoint(&ites, SelectItem(SelectItem::ENTITYROTATE, i), pos, ent.pos + makeAngle(2 * PI * ent.tank_ang_numer / ent.tank_ang_denom) * zpp * rotatehandle, secondnode * zpp * 1.5);
    } else {
      CHECK(0);
    }
  }
  
  CHECK(set<SelectItem>(ites.begin(), ites.end()).size() == ites.size());
  
  sort(ites.begin(), ites.end());
  
  return SelectStack(ites, select);
}
void Vecedit::pathprops(OtherState *ost) const {
  if(select.path == -1) {
    ost->hasPathProperties = false;
  } else {
    ost->hasPathProperties = true;
    ost->divisions = dv2.paths[select.path].dupes;
    ost->snowflakey = dv2.paths[select.path].reflect;
  }
}

bool Vecedit::changed() const {
  return modified;
}
ScrollBounds Vecedit::getScrollBounds(Float2 screenres, const WrapperState &state) const {
  ScrollBounds rv;
  
  // First, find the object bounds.
  rv.objbounds = dv2.boundingBox();
  
  // Second, find the screen bounds.
  rv.currentwindow = Float4(state.center - screenres * state.zpp / 2, state.center + screenres * state.zpp / 2);
  
  // Third, add some slack to the object bounds.
  rv.objbounds.sx -= rv.currentwindow.span_x() * 3 / 5;
  rv.objbounds.sy -= rv.currentwindow.span_y() * 3 / 5;
  rv.objbounds.ex += rv.currentwindow.span_x() * 3 / 5;
  rv.objbounds.ey += rv.currentwindow.span_y() * 3 / 5;
  
  addToBoundBox(&rv.objbounds, rv.currentwindow);
  
  return rv;
}

void drawLink(Float2 center, const vector<VectorPoint> &vpt, int j, float weight) {
  int k = (j + 1) % vpt.size();
  if(vpt[j].curvr) {
    CHECK(vpt[k].curvl);
    drawCurve(Float4(center + vpt[j].pos, center + vpt[j].pos + vpt[j].curvrp), Float4(center + vpt[k].pos + vpt[k].curvlp, center + vpt[k].pos), 50, weight);
  } else {
    drawLine(Float4(center + vpt[j].pos, center + vpt[k].pos), weight);
  }
}

static const Color selected = Color(1.0, 0.7, 0.7);
static const Color samepath = Color(0.7, 1.0, 0.7);
static const Color unselected_path = Color(0.7, 0.7, 1.0);
static const Color unselected_entity = Color(0.7, 1.0, 1.0);

int itemzone(const SelectItem &lhs) {
  if(lhs.type == SelectItem::PATHCENTER || lhs.type == SelectItem::PATHROTATE || lhs.type == SelectItem::NODE || lhs.type == SelectItem::CURVECONTROL || lhs.type == SelectItem::LINK) {
    return SelectItem::PATHCENTER;
  } else if(lhs.type == SelectItem::ENTITY || lhs.type == SelectItem::ENTITYROTATE) {
    return SelectItem::ENTITY;
  } else {
    CHECK(0);
  }
}

Color getUnselectedColor(const SelectItem &lhs) {
  if(itemzone(lhs) == SelectItem::PATHCENTER) {
    return unselected_path;
  } else if(itemzone(lhs) == SelectItem::ENTITY) {
    return unselected_entity;
  } else {
    CHECK(0);
  }
}

Color getAppropriateColor(const SelectItem &lhs, const SelectItem &rhs) {
  // this is complicated :(
  // If they are in different zones, it's unselected.
  // If they are the same zone, but different items, it's unselected.
  // If they are in the same zone, but the same item, but different actual selections, it's samepath.
  // If they are the same zone, same item, and same selections, it's selected.
  
  if(rhs.type == SelectItem::NONE)
    return getUnselectedColor(lhs);
  
  if(itemzone(lhs) != itemzone(rhs)) {
    return getUnselectedColor(lhs);
  }
  
  if(itemzone(lhs) == SelectItem::PATHCENTER) {
    if(lhs.path != rhs.path) {
      return getUnselectedColor(lhs);
    } else if(lhs.type != rhs.type || lhs.item != rhs.item) {
      return samepath;
    } else {
      return selected;
    }
  } else if(itemzone(lhs) == SelectItem::ENTITY) {
    if(lhs.entity == rhs.entity && lhs.type == rhs.type) {
      return selected;
    } else if(lhs.entity == rhs.entity) {
      return samepath;
    } else {
      return getUnselectedColor(lhs);
    }
  } else {
    CHECK(0);
  }
}

void setAppropriateLinkColor(const SelectItem &lhs, const SelectItem &rhs) {
  CHECK(lhs.type == SelectItem::CURVECONTROL);
  if(lhs.path != rhs.path) {
    CHECK(0); // this shouldn't even happen
  } else if(lhs.item != rhs.item || rhs.type == SelectItem::LINK || (rhs.type == SelectItem::CURVECONTROL && lhs.curveside != rhs.curveside)) {
    setColor(Color(0.5, 0.5, 0.5));
  } else {
    setColor(Color(1.0, 0.7, 0.7));
  }
}
  
void Vecedit::render(const WrapperState &state) const {
  setZoomCenter(state.center.x, state.center.y, state.zpp * getResolutionY() / 2);

  for(int i = 0; i < dv2.paths.size(); i++) {
    const VectorPath &vp = dv2.paths[i];
    for(int j = 0; j < vp.vpath.size(); j++) {
      setColor(getAppropriateColor(SelectItem(SelectItem::NODE, i, j), select));
      {
        double width;
        if(select.path == i) {
          width = primenode;
        } else {
          width = secondnode;
        }
        drawRectAround(vp.center + vp.vpath[j].pos, state.zpp * width, state.zpp);
      }
      
      if(select.path == i) {
        Float2 center = vp.center + vp.vpath[j].pos;
        if(vp.vpath[j].curvl) {
          Float2 fringe = vp.center + vp.vpath[j].pos + vp.vpath[j].curvlp;
          setAppropriateLinkColor(SelectItem(SelectItem::CURVECONTROL, i, j, false), select);
          drawLine(center, fringe, state.zpp);
          if(vp.vpath[j].flat) {
            Float2 shift = makeAngle(getAngle(fringe - center) + PI / 2) * state.zpp * 2;
            drawLine(center + shift, (center + fringe) / 2 + shift, state.zpp);
            drawLine(center - shift, (center + fringe) / 2 - shift, state.zpp);
          }
          setColor(getAppropriateColor(SelectItem(SelectItem::CURVECONTROL, i, j, false), select));
          drawRectAround(fringe, state.zpp * secondnode, state.zpp);
        }
        if(vp.vpath[j].curvr) {
          Float2 fringe = vp.center + vp.vpath[j].pos + vp.vpath[j].curvrp;
          setAppropriateLinkColor(SelectItem(SelectItem::CURVECONTROL, i, j, true), select);
          drawLine(center, fringe, state.zpp);
          if(vp.vpath[j].flat) {
            Float2 shift = makeAngle(getAngle(fringe - center) + PI / 2) * state.zpp * 2;
            drawLine(center + shift, (center + fringe) / 2 + shift, state.zpp);
            drawLine(center - shift, (center + fringe) / 2 - shift, state.zpp);
          }
          setColor(getAppropriateColor(SelectItem(SelectItem::CURVECONTROL, i, j, true), select));
          drawRectAround(fringe, state.zpp * secondnode, state.zpp);
        }
      }
      
      setColor(getAppropriateColor(SelectItem(SelectItem::LINK, i, j), select));
      drawLink(vp.center, vp.vpath, j, state.zpp * 2);
    }
    
    if(select.path == i) {
      setColor(getAppropriateColor(SelectItem(SelectItem::PATHCENTER, i), select));
      drawRectAround(vp.center, state.zpp * primenode, state.zpp);
     
      if(vp.reflect) {
        setColor(getAppropriateColor(SelectItem(SelectItem::PATHROTATE, i), select));
        drawRectAround(vp.center + makeAngle(2 * PI * vp.ang_numer / vp.ang_denom) * state.zpp * rotatehandle, state.zpp * secondnode, state.zpp);
        drawLine(vp.center, vp.center + makeAngle(2 * PI * vp.ang_numer / vp.ang_denom) * state.zpp * rotatehandle, state.zpp);
      }
    }
  }
  
  for(int i = 0; i < dv2.entities.size(); i++) {
    const Entity &ent = dv2.entities[i];
    
    if(ent.type == ENTITY_TANKSTART) {
      setColor(getAppropriateColor(SelectItem(SelectItem::ENTITY, i), select));
      drawLineLoop(defaultTank()->getTankVertices(Coord2(Coord(ent.pos.x), Coord(ent.pos.y)), (float)ent.tank_ang_numer / ent.tank_ang_denom * 2 * PI), state.zpp);
      if(select.entity == i) {
        setColor(getAppropriateColor(SelectItem(SelectItem::ENTITYROTATE, i), select));
        drawRectAround(ent.pos + makeAngle(2 * PI * ent.tank_ang_numer / ent.tank_ang_denom) * state.zpp * rotatehandle, state.zpp * secondnode, state.zpp);
        drawLine(ent.pos, ent.pos + makeAngle(2 * PI * ent.tank_ang_numer / ent.tank_ang_denom) * state.zpp * rotatehandle, state.zpp);
      }
    } else {
      CHECK(0);
    }
  }
  
  if(state.grid > 0) {
    setColor(0.2, 0.2, 0.5);
    drawGrid(state.grid, state.zpp);
  }
}

float toGrid(float x, float grid) {
  return floor(x / grid + 0.5) * grid;
}

Float2 toGrid(Float2 in, float grid) {
  in.x = toGrid(in.x, grid);
  in.y = toGrid(in.y, grid);
  return in;
}

bool rotateHelper(Float2 relative, int grid, int *numer, int *denom) {
  if(len(relative) != 0) {
    float ang = getAngle(relative) / 2 / PI;
    if(grid == -1) {
      *numer = round(ang * 3600);
      *denom = 3600;
    } else {
      *numer = modurot(round(ang * grid), grid);
      *denom = grid;
    }
    return true;
  } else {
    return false;
  }
}

// Okay let's have some logic!
// Possible states: 
// * Not selected
// * Selected
// * Selectednochange
// * Dragging

// Selection stack: Contains one item of each type at most, along with a "next item" to be used if the user ends up switching to the next item in the stack.

// Algorithm:
// * If the user has clicked, or released, generate a selection stack of things that they might be referring to. If they haven't, the selection stack contains solely the currently selected node. Select something if nothing is selected and switch to SELECTEDNEW mode. Otherwise, switch to SELECTED mode (which is pretty much mouse-down mode actually.)
// * If the user has clicked, handle the click in some way. Usually this only applies to right-clicks or when another tool is in use. Sometimes this will have to look through the selection stack to find something to apply it to, in which case we also change selection and change to SELECTEDNEW mode.
// * If the user has LMB down and has dragged, handle it if appropriate, possibly changing state to DRAGGING.
// * If the user has released, and we're in SELECTED mode, change to the next selection item as usual.

OtherState Vecedit::mouse(const MouseInput &mouse, const WrapperState &wrap) {
  OtherState ostate;
  ostate.ui = wrap.ui;

  Float2 worldlock = mouse.pos;
  if(wrap.grid > 0)
    worldlock = toGrid(worldlock, wrap.grid);
  bool compospush = mouse.b[0].push || mouse.b[1].push;
  bool composrelease = mouse.b[0].release || mouse.b[1].release;
  
  {
    SelectStack ss = getSelectionStack(mouse.pos, wrap.zpp);
    if(ss.hasItems()) {
      ostate.cursor = CURSOR_HAND;
    } else {
      ostate.cursor = CURSOR_NORMAL;
    }
  }

  // Push and initial selection
  if(compospush) {  // If the user has pushed a button . . .
    startposstack = getSelectionStack(mouse.pos, wrap.zpp);
    startpos = mouse.pos;
    if(!startposstack.hasItem(select)) { // and nothing in this stack is selected . . .
      select = startposstack.nextItem(); // select something . . .
      if(select.type != -1) { // and, if we actually got something . . 
        state = SELECTEDNOCHANGE; // select it, but don't allow it to be changed after buttonup.
        ostate.redraw = true;
      }
    } else {  // If something is selected already, just change to SELECTED mode.
      state = SELECTED;
    }
  }
  
  // Tool commands
  if(compospush && ostate.ui.newPath) { // Build a new path at this location
    VectorPath nvp;
    nvp.center = worldlock;
    
    nvp.path.push_back(VectorPoint());
    nvp.rebuildVpath();
    
    dv2.paths.push_back(nvp);
    select = SelectItem(SelectItem::NODE, dv2.paths.size() - 1, 0);
    modified = true;
    ostate.ui.newPath = false;
    ostate.redraw = true;
    state = DRAGGING;
    // we do NOT snapshot here because we'll snapshot once the player lets go of the button
  }
  
  if(compospush && ostate.ui.newNode && startposstack.hasItemType(SelectItem::LINK)) {
    select = startposstack.getItemType(SelectItem::LINK); // we'll change this again ASAP anyway
    
    VectorPath &path = dv2.paths[select.path];
    int newnode = path.vpathCreate((select.item + 1) % path.vpath.size());
    select.item = newnode;
    select.type = SelectItem::NODE;
    path.vpath[select.item].pos = worldlock - path.center;
    if(path.vpath[select.item].curvl)
      path.vpath[select.item].curvlp = (path.vpath[modurot(select.item - 1, path.vpath.size())].pos - path.vpath[select.item].pos) / 3;
    if(path.vpath[select.item].curvr)
      path.vpath[select.item].curvrp = (path.vpath[modurot(select.item + 1, path.vpath.size())].pos - path.vpath[select.item].pos) / 3;
    dv2.paths[select.path].vpathModify(select.item);
    modified = true;
    ostate.ui.newNode = false;
    ostate.redraw = true;
    state = DRAGGING;
    // we do NOT snapshot here because we'll snapshot once the player lets go of the button
  }
  
  if(compospush && ostate.ui.newTank) { // New tank creation
    Entity ne;
    ne.type = ENTITY_TANKSTART;
    
    ne.pos = worldlock;
    ne.tank_ang_numer = 0;
    ne.tank_ang_denom = 1;
    
    dv2.entities.push_back(ne);
    
    select = SelectItem(SelectItem::ENTITYROTATE, dv2.entities.size() - 1);
    modified = true;
    ostate.ui.newTank = false;
    ostate.redraw = true;
    state = DRAGGING;
    // we do NOT snapshot here because we'll snapshot once the player lets go of the button
  }
  
  // RMB
  if(mouse.b[1].push) {
    for(int i = 0; i < startposstack.itemOrder().size(); i++) {
      if(startposstack.itemOrder()[i].type == SelectItem::LINK) {
        select = startposstack.itemOrder()[i];
        state = SELECTED;
        dv2.paths[select.path].vpath[select.item].curvr = !dv2.paths[select.path].vpath[select.item].curvr;
        dv2.paths[select.path].vpathModify(select.item);
        modified = true;
        ostate.redraw = true;
        ostate.snapshot = true;
        break;
      } else if(startposstack.itemOrder()[i].type == SelectItem::NODE) {
        select = startposstack.itemOrder()[i];
        state = SELECTED;
        VectorPoint &vp = dv2.paths[select.path].vpath[select.item];
        vp.flat = !vp.flat;
        if(vp.flat) {
          float desang = (getAngle(vp.curvlp) + getAngle(vp.curvrp)) / 2;
          vp.curvlp = makeAngle(desang + PI * ((getAngle(vp.curvlp) > desang) * 2 - 1) / 2) * len(vp.curvlp);
          vp.curvrp = makeAngle(desang + PI * ((getAngle(vp.curvrp) >= desang) * 2 - 1) / 2) * len(vp.curvrp);
          dv2.paths[select.path].vpathModify(select.item);
          modified = true;
        }
        ostate.redraw = true;
        ostate.snapshot = true;
        break;
      }
    }
  }
  
  // Dragging
  if(mouse.b[0].down && ((state == SELECTED || state == SELECTEDNOCHANGE) && len(startpos - mouse.pos) > wrap.zpp * 3 || state == DRAGGING)) {
    // We want to grab something draggable if possible. However, if we've had our item overriden to one that didn't exist beforehand, we want to preserve that.
    if(!isDraggable(select.type)) {
      for(int i = 0; i < startposstack.itemOrder().size(); i++) {
        if(isDraggable(startposstack.itemOrder()[i].type)) {
          select = startposstack.itemOrder()[i];
        }
      }
    }
    
    // Actually move things
    if(select.type == SelectItem::NODE) {
      dv2.paths[select.path].vpath[select.item].pos = worldlock - dv2.paths[select.path].center;
      dv2.paths[select.path].vpathModify(select.item);
      modified = true;
      ostate.redraw = true;
      state = DRAGGING;
    } else if(select.type == SelectItem::CURVECONTROL) {
      Float2 destpt = worldlock - dv2.paths[select.path].center - dv2.paths[select.path].vpath[select.item].pos;
      VectorPoint &vp = dv2.paths[select.path].vpath[select.item];
      if(!select.curveside) {
        vp.curvlp = destpt;
        if(vp.flat) {
          vp.curvrp = makeAngle(getAngle(vp.curvlp) + PI) * len(vp.curvrp);
        }
      } else {
        vp.curvrp = destpt;
        if(vp.flat) {
          vp.curvlp = makeAngle(getAngle(vp.curvrp) + PI) * len(vp.curvlp);
        }
      }
      dv2.paths[select.path].vpathModify(select.item);
      modified = true;
      ostate.redraw = true;
      state = DRAGGING;
    } else if(select.type == SelectItem::PATHCENTER) {
      dv2.paths[select.path].center = worldlock;
      dv2.paths[select.path].moveCenterOrReflect();
      modified = true;
      ostate.redraw = true;
      state = DRAGGING;
    } else if(select.type == SelectItem::PATHROTATE) {
      // This is a bit complicated.
      if(rotateHelper(mouse.pos - dv2.paths[select.path].center, wrap.rotgrid, &dv2.paths[select.path].ang_numer, &dv2.paths[select.path].ang_denom)) {
        modified = true;
        ostate.redraw = true;
        state = DRAGGING;
      }
    } else if(select.type == SelectItem::ENTITY) {
      dv2.entities[select.entity].pos = worldlock;
      modified = true;
      ostate.redraw = true;
      state = DRAGGING;
    } else if(select.type == SelectItem::ENTITYROTATE) {
      if(rotateHelper(mouse.pos - dv2.entities[select.entity].pos, wrap.rotgrid, &dv2.entities[select.entity].tank_ang_numer, &dv2.entities[select.entity].tank_ang_denom)) {
        modified = true;
        ostate.redraw = true;
        state = DRAGGING;
      }
    } else if(isDraggable(select.type)) {
      CHECK(0); // *someone* forgot to implement some code >:(
    }
  }
  
  // Release
  if(composrelease) { // If the user has released a button . . .
    if(state == SELECTED) {  // and something was selected . . .
      select = startposstack.nextItem(); // select the next thing and redraw.
      state = IDLE;
      ostate.redraw = true;
    } else if(state == SELECTEDNOCHANGE) {  // and something was selected and we weren't supposed to change . . .
      state = IDLE; // don't.
    } else if(state == DRAGGING) {  // and we were dragging something . . .
      state = IDLE; // take a snapshot for undo.
      ostate.snapshot = true;
    } else if(state == IDLE) {
      dprintf("We were already idle, I'm kind of confused\n");
    } else {
      CHECK(0);
    }
  }
  
  pathprops(&ostate);
  return ostate;
}
OtherState Vecedit::del(const WrapperState &wrap) {
  OtherState ostate;
  ostate.ui = wrap.ui;

  if(select.type == SelectItem::NODE) {
    dv2.paths[select.path].vpathRemove(select.item);
    state = IDLE;
    select = SelectItem();
    modified = true;
    ostate.redraw = true;
    ostate.snapshot = true;
  } else if(select.type == SelectItem::PATHCENTER) {
    dv2.paths.erase(dv2.paths.begin() + select.path);
    state = IDLE;
    select = SelectItem();
    modified = true;
    ostate.redraw = true;
    ostate.snapshot = true;
  } else if(select.type == SelectItem::CURVECONTROL) {
    if(select.curveside == false)
      select.item = modurot(select.item - 1, dv2.paths[select.path].vpath.size()); // we can change this because we'll be clearing it soon anyway
    dv2.paths[select.path].vpath[select.item].curvr = !dv2.paths[select.path].vpath[select.item].curvr;
    dv2.paths[select.path].vpathModify(select.item);
    modified = true;
    ostate.redraw = true;
    ostate.snapshot = true;
    select = SelectItem();
  } else if(select.type == SelectItem::ENTITY || select.type == SelectItem::ENTITYROTATE) {
    dv2.entities.erase(dv2.entities.begin() + select.entity);
    modified = true;
    ostate.redraw = true;
    ostate.snapshot = true;
    select = SelectItem();
  }
  
  pathprops(&ostate);
  return ostate;
}
OtherState Vecedit::rotate(int reflects, const WrapperState &wrap) {
  OtherState ostate;
  ostate.ui = wrap.ui;
  ostate.redraw = true;
  ostate.snapshot = true;
  
  CHECK(select.path != -1);
  dv2.paths[select.path].dupes = reflects;
  dv2.paths[select.path].moveCenterOrReflect();
  
  pathprops(&ostate);
  return ostate;
}
OtherState Vecedit::snowflake(bool newstate, const WrapperState &wrap) {
  OtherState ostate;
  ostate.ui = wrap.ui;
  ostate.redraw = true;
  ostate.snapshot = true;
  
  CHECK(select.path != -1);
  dv2.paths[select.path].reflect = newstate;
  dv2.paths[select.path].moveCenterOrReflect();
  
  pathprops(&ostate);
  return ostate;
}

void Vecedit::clear() {
  *this = Vecedit();
  modified = false; 
}
void Vecedit::load(const string &filename) {
  clear();
  dv2 = loadDvec2(filename);
  modified = false;
}
bool Vecedit::save(const string &filename) {
  FILE *outfile;
  outfile = fopen(filename.c_str(), "wb");
  if(!outfile)
    return false;
      
  for(int i = 0; i < dv2.paths.size(); i++) {
    fprintf(outfile, "path {\n");
    fprintf(outfile, "  center=%f,%f\n", dv2.paths[i].center.x, dv2.paths[i].center.y);
    fprintf(outfile, "  reflect=%s\n", rf_names[dv2.paths[i].reflect]);
    fprintf(outfile, "  dupes=%d\n", dv2.paths[i].dupes);
    fprintf(outfile, "  angle=%d/%d\n", dv2.paths[i].ang_numer, dv2.paths[i].ang_denom);
    for(int j = 0; j < dv2.paths[i].path.size(); j++) {
      string lhs;
      string rhs;
      string straight;
      if(dv2.paths[i].path[j].curvl)
        lhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvlp.x, dv2.paths[i].path[j].curvlp.y);
      else
        lhs = "---";
      if(dv2.paths[i].path[j].curvr)
        rhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvrp.x, dv2.paths[i].path[j].curvrp.y);
      else
        rhs = "---";
      if(dv2.paths[i].path[j].curvl && dv2.paths[i].path[j].curvr && dv2.paths[i].path[j].flat)
        straight = " (straight)";
      fprintf(outfile, "  node= %s | %f,%f | %s%s\n", lhs.c_str(), dv2.paths[i].path[j].pos.x, dv2.paths[i].path[j].pos.y, rhs.c_str(), straight.c_str());
    }
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
  }
  for(int i = 0; i < dv2.entities.size(); i++) {
    fprintf(outfile, "entity {\n");
    fprintf(outfile, "  type=%s\n", ent_names[dv2.entities[i].type]);
    fprintf(outfile, "  coord=%f,%f\n", dv2.entities[i].pos.x, dv2.entities[i].pos.y);
    if(dv2.entities[i].type == ENTITY_TANKSTART) {
      fprintf(outfile, "  angle=%d/%d\n", dv2.entities[i].tank_ang_numer, dv2.entities[i].tank_ang_denom);
    } else {
      CHECK(0);
    }
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
  }
  fclose(outfile);
  
  modified = false;
  return true;
}

static Vecedit *emerg_save = NULL;

void doEmergSave() {
  CHECK(emerg_save);
  Vecedit *es = emerg_save;
  emerg_save = NULL;
  es->save("crash.dv2");
}

void Vecedit::registerEmergencySave() {
  CHECK(!emerg_save);
  dprintf("dvrcf\n");
  emerg_save = this;
  registerCrashFunction(&doEmergSave);
}

void Vecedit::unregisterEmergencySave() {
  CHECK(emerg_save);
  emerg_save = NULL;
  unregisterCrashFunction(&doEmergSave);
}

Vecedit::Vecedit() {
  modified = false;
  state = IDLE;
};
