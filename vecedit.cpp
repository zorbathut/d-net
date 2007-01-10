
#include "vecedit.h"

#include "gfx.h"
#include "util.h"

void drawCurveControls(const Float4 &ptah, const Float4 &ptbh, float spacing, float weight) {
  drawRectAround(ptah.s(), spacing, weight);
  drawRectAround(ptah.e(), spacing, weight);
  drawRectAround(ptbh.s(), spacing, weight);
  drawRectAround(ptbh.e(), spacing, weight);
  drawLine(ptah, weight);
  drawLine(ptbh, weight);
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

void Vecedit::render() const {
  setZoomCenter(center.x, center.y, zpp * getResolutionY() / 2);

  for(int i = 0; i < dv2.paths.size(); i++) {
    if(selected_path != i) {
      setColor(Color(0.7, 1.0, 0.7));
      drawVectorPath(dv2.paths[i], make_pair(Float2(0, 0), 1), 100, zpp * 2);
    } else {
      setColor(Color(1.0, 0.7, 0.7));
      drawVectorPath(dv2.paths[i], make_pair(Float2(0, 0), 1), 100, zpp * 2);
      for(int j = 0; j < dv2.paths[i].vpath.size(); j++)
        drawRectAround(dv2.paths[i].center + dv2.paths[i].vpath[j].pos, zpp * 10, zpp);
    }
  }
  
  setColor(0.2, 0.2, 0.5);
  drawGrid(32, zpp);
}
void Vecedit::mouse(const MouseInput &mouse) {
  Float2 world = (mouse.pos - Float2(getResolutionX() / 2, getResolutionY() / 2)) * zpp + Float2(center);
  
  if(mouse.b[0].push) {
    dprintf("Worldpos is %f,%f from %f,%f\n", world.x, world.y, mouse.pos.x, mouse.pos.y);
    selected_path = 0;
    resync_gui_callback->Run();
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
  *this = Vecedit(resync_gui_callback);
  resync_gui_callback->Run();
}
void Vecedit::load(const string &filename) {
  dv2 = loadDvec2(filename);
  selected_path = -1;
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

Vecedit::Vecedit(const smart_ptr<Closure<> > &resync_gui_callback) : resync_gui_callback(resync_gui_callback) {
  center = Float2(0, 0);
  zpp = 0.25;
  selected_path = -1;
};
