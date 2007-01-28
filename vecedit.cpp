
#include "vecedit.h"

#include "gfx.h"
#include "util.h"

const float primenode = 8;
const float secondnode = 5;

Selectitem::Selectitem() {
  type = NONE;
  path = -1;
  item = -1;
  curveside = false;
}

Selectitem::Selectitem(int type, int path) : type(type), path(path), item(0), curveside(false) {
  CHECK(type == PATHCENTER);
};
Selectitem::Selectitem(int type, int path, int item) : type(type), path(path), item(item), curveside(false) {
  CHECK(type == NODE || type == LINK);
};
Selectitem::Selectitem(int type, int path, int item, bool curveside) : type(type), path(path), item(item), curveside(curveside) {
  CHECK(type == CURVECONTROL);
};

OtherState::OtherState() {
  cursor = CURSOR_UNCHANGED;
  redraw = false;
}

bool operator<(const Selectitem &lhs, const Selectitem &rhs) {
  if(lhs.type != rhs.type) return lhs.type < rhs.type;
  if(lhs.path != rhs.path) return lhs.path < rhs.path;
  if(lhs.item != rhs.item) return lhs.item < rhs.item;
  if(lhs.curveside != rhs.curveside) return lhs.curveside < rhs.curveside;
  return false;
}

bool operator==(const Selectitem &lhs, const Selectitem &rhs) {
  if(lhs.type != rhs.type) return false;
  if(lhs.path != rhs.path) return false;
  if(lhs.item != rhs.item) return false;
  if(lhs.curveside != rhs.curveside) return false;
  return true;
}

void maybeAddPoint(vector<Selectitem> *ite, Selectitem tite, Float2 pos, Float2 loc, float dist) {
  if(max(abs(pos.x - loc.x), abs(pos.y - loc.y)) <= dist)
    ite->push_back(tite);
}

void maybeAddLine(vector<Selectitem> *ite, Selectitem tite, Float2 pos, Float4 loc, float dist) {
  if(linepointdistance(loc, pos) <= dist)
    ite->push_back(tite);
}

void maybeAddCurve(vector<Selectitem> *ite, Selectitem tite, Float2 pos, Float4 ptah, Float4 ptbh, float dist) {
  vector<Float2> curv = generateCurve(ptah, ptbh, 20);
  for(int i = 0; i < curv.size() - 1; i++) {
    if(linepointdistance(Float4(curv[i], curv[i + 1]), pos) <= dist) {
      ite->push_back(tite);
      break;
    }
  }
}

vector<Selectitem> Vecedit::getSelectionStack(Float2 pos) const {
  vector<Selectitem> ites;
  
  for(int i = 0; i < dv2.paths.size(); i++) {
    const VectorPath &vp = dv2.paths[i];
    const bool thisselect = (i == select.path);
    for(int j = 0; j < vp.vpath.size(); j++) {
      float selsize;
      if(thisselect) {
        selsize = primenode * zpp;
      } else {
        selsize = secondnode * zpp;
      }
      
      maybeAddPoint(&ites, Selectitem(Selectitem::NODE, i, j), pos, vp.center + vp.vpath[j].pos, selsize * 1.5);
      
      if(thisselect && vp.vpath[j].curvl) {
        maybeAddPoint(&ites, Selectitem(Selectitem::CURVECONTROL, i, j, false), pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvlp, secondnode * zpp * 1.5);
      }
      if(thisselect && vp.vpath[j].curvr) {
        maybeAddPoint(&ites, Selectitem(Selectitem::CURVECONTROL, i, j, true), pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvrp, secondnode * zpp * 1.5);
      }
      
      if(thisselect) {
        maybeAddPoint(&ites, Selectitem(Selectitem::PATHCENTER, i), pos, vp.center, primenode * zpp * 1.5);
      }
      
      int k = (j + 1) % vp.vpath.size();
      if(vp.vpath[j].curvr) {
        CHECK(vp.vpath[k].curvl);
        maybeAddCurve(&ites, Selectitem(Selectitem::LINK, i, j), pos, Float4(vp.center + vp.vpath[j].pos, vp.center + vp.vpath[j].pos + vp.vpath[j].curvrp), Float4(vp.center + vp.vpath[k].pos + vp.vpath[k].curvlp, vp.center + vp.vpath[k].pos), selsize);
      } else {
        maybeAddLine(&ites, Selectitem(Selectitem::LINK, i, j), pos, Float4(vp.center + vp.vpath[j].pos, vp.center + vp.vpath[k].pos), selsize);
      }
    }
  }
  
  sort(ites.begin(), ites.end());
  
  return ites;
}

bool Vecedit::changed() const {
  return modified;
}
ScrollBounds Vecedit::getScrollBounds(Float2 screenres) const {
  ScrollBounds rv;
  
  // First, find the object bounds.
  rv.objbounds = dv2.boundingBox();
  
  // Second, find the screen bounds.
  rv.currentwindow = Float4(center.x - zpp * screenres.x / 2, center.y - zpp * screenres.y / 2, center.x + zpp * screenres.x / 2, center.y + zpp * screenres.y / 2);
  
  // Third, add some slack to the object bounds.
  rv.objbounds.sx -= rv.currentwindow.span_x() * 3 / 5;
  rv.objbounds.sy -= rv.currentwindow.span_y() * 3 / 5;
  rv.objbounds.ex += rv.currentwindow.span_x() * 3 / 5;
  rv.objbounds.ey += rv.currentwindow.span_y() * 3 / 5;
  
  addToBoundBox(&rv.objbounds, rv.currentwindow);
  
  return rv;
}
void Vecedit::setScrollPos(Float2 scrollpos) {
  center = scrollpos;
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

void setAppropriateColor(const Selectitem &lhs, const Selectitem &rhs) {
  if(lhs.path != rhs.path) {
    setColor(Color(0.7, 0.7, 1.0));
  } else if(lhs.type != rhs.type || lhs.item != rhs.item) {
    setColor(Color(0.7, 1.0, 0.7));
  } else {
    setColor(Color(1.0, 0.7, 0.7));
  }
}

void setAppropriateLinkColor(const Selectitem &lhs, const Selectitem &rhs) {
  CHECK(lhs.type == Selectitem::CURVECONTROL);
  if(lhs.path != rhs.path) {
    CHECK(0); // this shouldn't even happen
  } else if(lhs.item != rhs.item || rhs.type == Selectitem::LINK || (rhs.type == Selectitem::CURVECONTROL && lhs.curveside != rhs.curveside)) {
    setColor(Color(0.5, 0.5, 0.5));
  } else {
    setColor(Color(1.0, 0.7, 0.7));
  }
}
  
void Vecedit::render() const {
  setZoomCenter(center.x, center.y, zpp * getResolutionY() / 2);

  for(int i = 0; i < dv2.paths.size(); i++) {
    for(int j = 0; j < dv2.paths[i].vpath.size(); j++) {
      setAppropriateColor(Selectitem(Selectitem::NODE, i, j), select);
      {
        double width;
        if(select.path == i) {
          width = primenode;
        } else {
          width = secondnode;
        }
        drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, zpp * width, zpp);
      }
      
      if(select.path == i) {
        if(dv2.paths[i].vpath[j].curvl) {
          setAppropriateLinkColor(Selectitem(Selectitem::CURVECONTROL, i, j, false), select);
          drawLine(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvlp, zpp);
          setAppropriateColor(Selectitem(Selectitem::CURVECONTROL, i, j, false), select);
          drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvlp, zpp * secondnode, zpp);
        }
        if(dv2.paths[i].vpath[j].curvr) {
          setAppropriateLinkColor(Selectitem(Selectitem::CURVECONTROL, i, j, true), select);
          drawLine(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvrp, zpp);
          setAppropriateColor(Selectitem(Selectitem::CURVECONTROL, i, j, true), select);
          drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvrp, zpp * secondnode, zpp);
        }
        
        setAppropriateColor(Selectitem(Selectitem::PATHCENTER, i), select);
        drawRectAround(dv2.paths[i].center, zpp * primenode, zpp);
      }
      
      setAppropriateColor(Selectitem(Selectitem::LINK, i, j), select);
      drawLink(dv2.paths[i].center, dv2.paths[i].vpath, j, zpp * 2);
    }
  }
  
  setColor(0.2, 0.2, 0.5);
  drawGrid(grid, zpp);
}

void selectThings(Selectitem *select, const vector<Selectitem> &ss) {
  CHECK(ss.size());
  if(count(ss.begin(), ss.end(), *select)) {
    *select = ss[(find(ss.begin(), ss.end(), *select) - ss.begin() + 1) % ss.size()];
  } else {
    *select = ss[0];
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

// Okay let's have some logic!
// Possible states: 
// * Not selected
// * Selected
// * SELECTEDNEW
// * Dragging

// * This stack isn't selected + mousedown + something selectable -> Selectednew
// * Previouslyselected + mouseup + something selectable -> rotate through selectable things
// * Selectednew | Selected + mousecurrentlydown + selected draggable + movement -> dragging
// * Dragging + mouseup -> back to selected
// Note that dragging + mouseup != rotate

// * Not selected + rightmousedown + something selectable -> select it, toggle
// * Selected + rightmousedown -> toggle

// For selecting things: order in the following direction!
// * Points that are close enough, sorted by path, then by point
// * Lines that are close enough

OtherState Vecedit::mouse(const MouseInput &mouse) {
  ostate.redraw = false;
  
  Float2 world = (mouse.pos - Float2(getResolutionX() / 2, getResolutionY() / 2)) * zpp + Float2(center);
  Float2 worldlock = world;
  worldlock = toGrid(worldlock, grid);
  
  {
    vector<Selectitem> ss = getSelectionStack(world);
    
    if(!ss.size()) {
      ostate.cursor = CURSOR_NORMAL;
      
      if(mouse.b[0].push) {
        select = Selectitem();
        ostate.redraw = true;
      }
      
      if(mouse.b[0].release) {
        state = IDLE;
      } // whatever we were doing, we're not doing it now
    } else {
      ostate.cursor = CURSOR_HAND;
      
      if(mouse.b[0].push) {
        if(state == IDLE) {
          if(count(ss.begin(), ss.end(), select)) {
            state = SELECTED;
          } else {
            selectThings(&select, ss);
            state = SELECTEDNEW;
            ostate.redraw = true;
          }
          startpos = world;
        }
      }
      
      if(mouse.b[0].release) {
        if(state == SELECTED) {
          selectThings(&select, ss);
          state = IDLE;
          ostate.redraw = true;
        } else if(state == SELECTEDNEW || state == DRAGGING) {
          state = IDLE;
        }
      }
    }
    
    if((state == SELECTED || state == SELECTEDNEW) && len(startpos - world) > zpp * 3 || state == DRAGGING) {
      if(select.type == Selectitem::NODE) {
        dv2.paths[select.path].vpath[select.item].pos = worldlock - dv2.paths[select.path].center;
        dv2.paths[select.path].vpathModify(select.item);
        state = DRAGGING;
        modified = true;
        ostate.redraw = true;
      } else if(select.type == Selectitem::CURVECONTROL) {
        Float2 destpt = worldlock - dv2.paths[select.path].center - dv2.paths[select.path].vpath[select.item].pos;
        VectorPoint &vp = dv2.paths[select.path].vpath[select.item];
        if(!select.curveside) {
          vp.curvlp = destpt;
        } else {
          vp.curvrp = destpt;
        }
        dv2.paths[select.path].vpathModify(select.item);
        state = DRAGGING;
        modified = true;
        ostate.redraw = true;
      } else if(select.type == Selectitem::PATHCENTER) {
        dv2.paths[select.path].center = worldlock;
        dv2.paths[select.path].moveCenterOrReflect();
        state = DRAGGING;
        modified = true;
        ostate.redraw = true;
      }
    }
  }

  if(mouse.dw != 0) {
    const float mult = pow(1.2, mouse.dw / 120.0);
    
    zpp /= mult;
    
    if(mult > 1.0)
      center = world - (world - center) / mult;
    
    ostate.redraw = true;
  }
  
  return ostate;
}
OtherState Vecedit::gridupd(int in_grid) {
  grid = in_grid;
  ostate.redraw = true;
  return ostate;
}

void Vecedit::clear() {
  int tgrid = grid;
  *this = Vecedit();
  grid = tgrid;
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
      if(dv2.paths[i].path[j].curvl)
        lhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvlp.x, dv2.paths[i].path[j].curvlp.y);
      else
        lhs = "---";
      if(dv2.paths[i].path[j].curvr)
        rhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvrp.x, dv2.paths[i].path[j].curvrp.y);
      else
        rhs = "---";
      fprintf(outfile, "  node= %s | %f,%f | %s\n", lhs.c_str(), dv2.paths[i].path[j].pos.x, dv2.paths[i].path[j].pos.y, rhs.c_str());
    }
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
  }
  for(int i = 0; i < dv2.entities.size(); i++) {
    fprintf(outfile, "entity {\n");
    fprintf(outfile, "  type=%s\n", ent_names[dv2.entities[i].type]);
    fprintf(outfile, "  coord=%f,%f\n", dv2.entities[i].pos.x, dv2.entities[i].pos.y);
    for(int j = 0; j < dv2.entities[i].params.size(); j++)
      fprintf(outfile, "%s", dv2.entities[i].params[j].dumpTextRep().c_str());
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
  center = Float2(0, 0);
  zpp = 0.25;
  
  modified = false;
  state = IDLE;
};
