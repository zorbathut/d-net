
#include "vecedit.h"

#include "gfx.h"
#include "util.h"

const float primenode = 8;
const float secondnode = 5;

Selectitem::Selectitem() {
  type = NONE;
  path = -1;
  item = -1;
}
Selectitem::Selectitem(int type, int path, int item) : type(type), path(path), item(item) {
};

bool operator<(const Selectitem &lhs, const Selectitem &rhs) {
  if(lhs.type != rhs.type) return lhs.type < rhs.type;
  if(lhs.path != rhs.path) return lhs.path < rhs.path;
  if(lhs.item != rhs.item) return lhs.item < rhs.item;
  return false;
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
  return false;
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
  resync_gui_callback->Run();
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
          drawLine(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvlp, zpp);
          drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvlp, zpp * secondnode, zpp);
        }
        if(dv2.paths[i].vpath[j].curvr) {
          drawLine(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvrp, zpp);
          drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos + dv2.paths[i].vpath[j].curvrp, zpp * secondnode, zpp);
        }
      }
      
      setAppropriateColor(Selectitem(Selectitem::LINK, i, j), select);
      drawLink(dv2.paths[i].center, dv2.paths[i].vpath, j, zpp * 2);
    }
  }
  
  setColor(0.2, 0.2, 0.5);
  drawGrid(32, zpp);
}

// Okay let's have some logic!
// Possible states: 
// * Not selected
// * Selected
// * Dragging

// * This stack isn't selected + mousedown + something selectable -> selected
// * Selected + mouseup + something selectable -> rotate through selectable things
// * Selected + mousecurrentlydown + selected draggable + movement -> dragging
// * Dragging + mouseup -> back to selected
// Note that dragging + mouseup != rotate

// * Not selected + rightmousedown + something selectable -> select it, toggle
// * Selected + rightmousedown -> toggle

// For selecting things: order in the following direction!
// * Points that are close enough, sorted by path, then by point
// * Lines that are close enough

void Vecedit::mouse(const MouseInput &mouse) {
  Float2 world = (mouse.pos - Float2(getResolutionX() / 2, getResolutionY() / 2)) * zpp + Float2(center);
  
  {
    vector<Selectitem> ss = getSelectionStack(world);
    if(!ss.size()) {
      cursor_change_callback->Run(CURSOR_NORMAL);
    } else if(ss[0].type == Selectitem::NODE) {
      cursor_change_callback->Run(CURSOR_HAND);
      if(mouse.b[0].down) {
        select = ss[0];
        resync_gui_callback->Run();
      }
    } else if(ss[0].type == Selectitem::LINK) {
      cursor_change_callback->Run(CURSOR_CROSS);
      if(mouse.b[0].down) {
        select = ss[0];
        resync_gui_callback->Run();
      }
    } else {
      CHECK(0); // well fuck
    }
  }
  
  if(state == IDLE) {
    
  }

  if(mouse.dw == 0)
    return;
  
  const float mult = pow(1.2, mouse.dw / 120.0);
  
  zpp /= mult;
  
  if(mult > 1.0)
    center = world - (world - center) / mult;
  
  resync_gui_callback->Run();
}

void Vecedit::clear() {
  *this = Vecedit(resync_gui_callback, cursor_change_callback);
  resync_gui_callback->Run();
}
void Vecedit::load(const string &filename) {
  clear();
  dv2 = loadDvec2(filename);
  resync_gui_callback->Run();
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
  
  return true;
}

Vecedit::Vecedit(const smart_ptr<Closure<> > &resync_gui_callback, const smart_ptr<Closure<Cursor> > &cursor_change_callback) : resync_gui_callback(resync_gui_callback), cursor_change_callback(cursor_change_callback) {
  center = Float2(0, 0);
  zpp = 0.25;
  
  state = IDLE;
};
