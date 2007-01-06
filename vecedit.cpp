
#include "vecedit.h"

#include "gfx.h"
#include "util.h"

bool Vecedit::changed() const {
  return false;
}

void Vecedit::render() const {
  setZoomCenter(xc, yc, zpp * getResolutionY() / 2);
  setColor(Color(0.7, 1.0, 0.7));

  for(int i = 0; i < dv2.paths.size(); i++)
    drawVectorPath(dv2.paths[i], make_pair(Float2(0, 0), 1), 100, zpp * 2);
  
  setColor(0.2, 0.2, 0.5);
  drawGrid(32, zpp);
}
void Vecedit::mouse(const MouseInput &mouse) {
  dprintf("Mouse movement! Now at %d,%d, wheel %d, buttons %s %s %s\n", mouse.x, mouse.y, mouse.dw, mouse.b[0].stringize().c_str(), mouse.b[1].stringize().c_str(), mouse.b[2].stringize().c_str());
  
}

void Vecedit::clear() {
  dv2 = Dvec2();
  resync_gui_callback->Run();
}
void Vecedit::load(const string &filename) {
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
    fprintf(outfile, "  center=%f,%f\n", dv2.paths[i].centerx, dv2.paths[i].centery);
    fprintf(outfile, "  reflect=%s\n", rf_names[dv2.paths[i].reflect]);
    fprintf(outfile, "  dupes=%d\n", dv2.paths[i].dupes);
    fprintf(outfile, "  angle=%d/%d\n", dv2.paths[i].ang_numer, dv2.paths[i].ang_denom);
    for(int j = 0; j < dv2.paths[i].path.size(); j++) {
      string lhs;
      string rhs;
      if(dv2.paths[i].path[j].curvl)
        lhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvlx, dv2.paths[i].path[j].curvly);
      else
        lhs = "---";
      if(dv2.paths[i].path[j].curvr)
        rhs = StringPrintf("%f,%f", dv2.paths[i].path[j].curvrx, dv2.paths[i].path[j].curvry);
      else
        rhs = "---";
      fprintf(outfile, "  node= %s | %f,%f | %s\n", lhs.c_str(), dv2.paths[i].path[j].x, dv2.paths[i].path[j].y, rhs.c_str());
    }
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
  }
  for(int i = 0; i < dv2.entities.size(); i++) {
    fprintf(outfile, "entity {\n");
    fprintf(outfile, "  type=%s\n", ent_names[dv2.entities[i].type]);
    fprintf(outfile, "  coord=%f,%f\n", dv2.entities[i].x, dv2.entities[i].y);
    for(int j = 0; j < dv2.entities[i].params.size(); j++)
      fprintf(outfile, "%s", dv2.entities[i].params[j].dumpTextRep().c_str());
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
  }
  fclose(outfile);
  
  return true;
}

Vecedit::Vecedit(const smart_ptr<Closure0> &resync_gui_callback) : resync_gui_callback(resync_gui_callback) {
  xc = 0;
  yc = 0;
  zpp = 0.25;
};
