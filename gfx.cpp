
#include "gfx.h"

#include "args.h"
#include "coord.h"
#include "parse.h"
#include "util.h"
#include "debug.h"
#include "rng.h"

#include <fstream>

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

using namespace std;

/*************
 * Expensive object pool
 */

template<typename T> class Pool : boost::noncopyable {
public:
  T *acquire() {
    if(!poolitems.empty()) {
      T *item = poolitems.back();
      poolitems.pop_back();
      return item;
    } else {
      return new T;
    }
  }
  void free(T *item) {
    poolitems.push_back(item);
  }
  
  Pool() { }
  ~Pool() {
    for(int i = 0; i < poolitems.size(); i++)
      delete poolitems[i];
  }
  
private:
  vector<T*> poolitems;
};

template<typename T> class PoolObj : boost::noncopyable{
public:
  T *operator->() {
    return item;
  }
  T &operator*() {
    return *item;
  }

  PoolObj() {
    item = pool.acquire();
  }
  ~PoolObj() {
    item->clear();
    pool.free(item);
  }

private:
  T *item;

  static Pool<T> pool;
};

template<typename T> Pool<T> PoolObj<T>::pool;
  
/*************
 * Infrastructure
 */

vector<int> getView() {
  GLint gli[4];
  glGetIntegerv(GL_VIEWPORT, gli);
  vector<int> rv(gli, gli + 4);
  return rv;
}

int getResolutionX() {
  return getView()[2];
}
int getResolutionY() {
  return getView()[3];
}
 
class GfxWindowState {
public:
  float saved_sx;
  float saved_sy;
  float saved_ey;
  float fade;

  Float4 newbounds;

  GfxWindow *gfxw;

  void setScissor() const;
};
static vector<GfxWindowState> windows;

static float map_sx;
static float map_sy;
static float map_ex;
static float map_ey;
static float map_zoom;
static Float4 map_bounds;

static float map_saved_sx;
static float map_saved_sy;
static float map_saved_ey;

class FontCharacter {
public:
  vector<vector<pair<int, int> > > art;
  int width;

  void normalize() {
    int sx = 1000;
    int ex = -1000;
    
    for(int i = 0; i < art.size(); i++) {
      for(int j = 0; j < art[i].size(); j++) {
        sx = min(sx, art[i][j].first);
        ex = max(ex, art[i][j].first);
      }
    }
    
    if(sx == 1000 && ex == -1000) {
      CHECK(art.size() == 0);
      sx = 0;
      ex = 4;
    } else {
      ex++;
      if(ex <= sx)
        dprintf("%d, %d", sx, ex);
      CHECK(ex > sx);
    }
    
    for(int i = 0; i < art.size(); i++)
      for(int j = 0; j < art[i].size(); j++)
        art[i][j].first -= sx;
    width = ex - sx;
  }
};

static map<char, FontCharacter> fontdata;

void initGfx() {
  // Load fonts. This used to include more code.
  CHECK(fontdata.size() == 0);
  ifstream font("data/font.dwh");
  CHECK(font);
  kvData kvd;
  while(getkvData(font, &kvd)) {
    CHECK(kvd.category == "character");
    CHECK(kvd.kv.count("id"));
    string id = kvd.consume("id");
    if(id == "hash")
      id = "#";
    if(id == "space")
      id = " ";
    CHECK(id.size() == 1);
    vector<string> paths = tokenize(kvd.consume("path"), "\n");
    
    CHECK(!fontdata.count(id[0]));
    fontdata[id[0]]; // creates it for "space", which will have no paths
    for(int i = 0; i < paths.size(); i++) {
      vector<string> order = tokenize(paths[i], " ");
      CHECK(order.size() >= 2);
      vector<pair<int, int> > tpath;
      for(int j = 0; j < order.size(); j++) {
        vector<int> out = sti(tokenize(order[j], ","));
        CHECK(out.size() == 2);
        tpath.push_back(make_pair(out[0], out[1]));
      }
      CHECK(tpath.size() >= 2);
      fontdata[id[0]].art.push_back(tpath);
    }
    
    if(kvd.kv.count("width")) {
      fontdata[id[0]].width = atoi(kvd.consume("width").c_str());
    } else {
      fontdata[id[0]].normalize();
    }
    
    kvd.shouldBeDone();
  }
}

void updateResolution(float aspect) {
  // Set up our OpenGL translation so we have the right image size
  {
    glLoadIdentity();
    GLfloat flipy[16]= { 2 / aspect, 0, 0, 0,   0, -2, 0, 0,   0, 0, 1, 0,  -1, -1, 0, 1 };
    glMultMatrixf(flipy);
    glTranslatef(0, -1, 0);
  }
  
  // Set up our windowing system
  if(windows.size() == 1)
    windows.clear();
  
  {
    CHECK(windows.size() == 0);
    GfxWindowState gfws;
    gfws.saved_sx = 0;
    gfws.saved_sy = 0;
    gfws.saved_ey = 1;
    gfws.fade = 1;

    gfws.newbounds = Float4(0, 0, aspect, 1);
    gfws.gfxw = NULL;
    
    windows.push_back(gfws);
    windows.back().setScissor();
  }
}

float curWeight = -1.f;
int lineCount = 0;
int clusterCount = 0;
int lpFailure = 0;
int lpSuccess = 0;
int lastStats = 0;
int renderedFrameId = 0;
int totalLineCount = 0;

static Color curcolor;
static Color clearcolor;
const Float2 invPoint(-1234e12, 394e9);
Float2 lastPoint = invPoint;

string printGraphicsStats() {
  string gstat = StringPrintf("%.2f/%.2f average clust/lines in %d frames, %d/%d strip hit (%.2f%%)", clusterCount / float(renderedFrameId - lastStats), totalLineCount / float(renderedFrameId - lastStats), renderedFrameId - lastStats, lpSuccess, lpFailure + lpSuccess, lpSuccess / float(lpFailure + lpSuccess) * 100);
  clusterCount = 0;
  totalLineCount = 0;
  lpFailure = 0;
  lpSuccess = 0;
  lastStats = renderedFrameId;
  return gstat;
}

void beginLineCluster(float weight) {
  CHECK(glGetError() == GL_NO_ERROR);
  CHECK(curWeight == -1.f);
  CHECK(weight != -1.f);
  glLineWidth(weight / map_zoom * getResolutionY());   // GL uses pixels internally for this unit, so I have to translate from game-meters
  CHECK(glGetError() == GL_NO_ERROR);
  glBegin(GL_LINE_STRIP);
  curWeight = weight;
  lineCount = 0;
  lastPoint = invPoint;
  clusterCount++;
}

void finishLineCluster() {
  if(curWeight != -1.f) {
    curWeight = -1.f;
    glEnd();
    lineCount = 0;
  }
  CHECK(glGetError() == GL_NO_ERROR);
  glLineWidth(1);
}

static bool frame_running = false;

void initFrame() {
  CHECK(!frame_running);
  CHECK(curWeight == -1.f);
  {
    GLint v = 0;
    glGetIntegerv(GL_STENCIL_BITS, &v);
    CHECK(v >= 1);
  }
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_SCISSOR_TEST);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  //glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
  //glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glStencilMask(1);
  clearFrame(Color(0.05, 0.05, 0.05));
  CHECK(glGetError() == GL_NO_ERROR);
  frame_running = true;
  glStencilMask(0);
  
  registerCrashFunction(deinitFrame);
}

void clearFrame(const Color &color) {
  finishLineCluster();
  glClearColor(color.r, color.g, color.b, 0.0f);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  CHECK(glGetError() == GL_NO_ERROR);
  clearcolor = color;
}

bool frameRunning() {
  return frame_running;
}

void deinitFrame() {
  unregisterCrashFunction(deinitFrame);
  
  CHECK(frame_running);
  finishLineCluster();
  glFlush();
  CHECK(glGetError() == GL_NO_ERROR);
  renderedFrameId++;
  frame_running = false;
}

GfxWindow::GfxWindow(const Float4 &obounds, float fade) {
  finishLineCluster();
  
  Float4 bounds = obounds;
  bounds.sx = max(bounds.sx, map_bounds.sx);
  bounds.sy = max(bounds.sy, map_bounds.sy);
  bounds.ex = min(bounds.ex, map_bounds.ex);
  bounds.ey = min(bounds.ey, map_bounds.ey);
  if(!bounds.isNormalized()) {
    dprintf("WARNING: Null gfxwindow!\n");
    bounds = Float4(map_bounds.sx, map_bounds.sy, map_bounds.sx, map_bounds.sy);
  }
  CHECK(bounds.isNormalized());
  
  GfxWindowState gfws;
  gfws.saved_sx = map_saved_sx;
  gfws.saved_sy = map_saved_sy;
  gfws.saved_ey = map_saved_ey;
  gfws.fade = windows.back().fade * fade;

  // newbounds is the location on the screen that the new bounds fill
  gfws.newbounds = Float4((bounds.sx - map_sx) / map_zoom, (bounds.sy - map_sy) / map_zoom, (bounds.ex - map_sx) / map_zoom, (bounds.ey - map_sy) / map_zoom);
  
  gfws.gfxw = this;
  
  map_bounds = bounds;
  
  windows.push_back(gfws);
  
  windows.back().setScissor();
}
GfxWindow::~GfxWindow() {
  CHECK(windows.size() > 1);
  CHECK(windows.back().gfxw == this);
  float tmap_sx = windows.back().saved_sx;
  float tmap_sy = windows.back().saved_sy;
  float tmap_ey = windows.back().saved_ey;
  windows.pop_back();
  setZoomVertical(tmap_sx, tmap_sy, tmap_ey);
  
  windows.back().setScissor();
}

void GfxWindowState::setScissor() const { // yes, the Y's are correct - the screen coordinates are (0,0)-(1,aspect).
  int sx = int(getResolutionY() * newbounds.sx);
  int sy = int(getResolutionY() * newbounds.sy);
  int ex = int(ceil(getResolutionY() * newbounds.ex));
  int ey = int(ceil(getResolutionY() * newbounds.ey));
  glScissor(sx, getResolutionY() - ey, ex - sx, ey - sy);
}

bool stenciled = false;
GfxStenciled::GfxStenciled() {
  CHECK(!stenciled);
  finishLineCluster();
  CHECK(glGetError() == GL_NO_ERROR);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_EQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  CHECK(glGetError() == GL_NO_ERROR);
  stenciled = true;
}

GfxStenciled::~GfxStenciled() {
  CHECK(stenciled);
  finishLineCluster();
  CHECK(glGetError() == GL_NO_ERROR);
  glDisable(GL_STENCIL_TEST);
  CHECK(glGetError() == GL_NO_ERROR);
  stenciled = false;
}

bool inverting = false;
GfxInvertingStencil::GfxInvertingStencil() {
  CHECK(!inverting);
  
  finishLineCluster();
  CHECK(glGetError() == GL_NO_ERROR);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glEnable(GL_STENCIL_TEST);
  glStencilMask(1);
  glStencilFunc(GL_ALWAYS, 0, 0);
  glStencilOp(GL_INVERT, GL_INVERT, GL_INVERT);
  CHECK(glGetError() == GL_NO_ERROR);
  
  inverting = true;
}

GfxInvertingStencil::~GfxInvertingStencil() {
  CHECK(inverting);
  CHECK(curWeight == -1.f);
  
  glDisable(GL_STENCIL_TEST);
  glStencilMask(0);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  CHECK(glGetError() == GL_NO_ERROR);

  inverting = false;
}

/*************
 * Primitives
 */

void setZoom(const Float4 &fl4) {
  float rat = fl4.span_x() / fl4.span_y() / getAspect();
  if(rat < 0.999 || rat > 1.001) {
    dprintf("rat is %f\n", rat);
    CHECK(0);
  }
  setZoomVertical(fl4.sx, fl4.sy, fl4.ey);
}

void setZoomAround(const CFC4 &bbox) {
  Float2 center = bbox->midpoint();
  float zoomtop = bbox->span_y() / 2;
  float zoomside = bbox->span_x() / 2;
  float zoomtopfinal = max(zoomtop, zoomside / getAspect());
  setZoomVertical(center.x - zoomtopfinal * getAspect(), center.y - zoomtopfinal, center.y + zoomtopfinal);
}

void setZoomCenter(float cx, float cy, float radius_y) {
  setZoomVertical(cx - radius_y * getAspect(), cy - radius_y, cy + radius_y);
}

void setZoomVertical(float in_sx, float in_sy, float in_ey) {
  finishLineCluster();
  
  map_saved_sx = in_sx;
  map_saved_sy = in_sy;
  map_saved_ey = in_ey;
  
  map_bounds = Float4(in_sx, in_sy, in_sx + (in_ey - in_sy) * getAspect(), in_ey);
  
  float real_sy = in_sy - ((in_ey - in_sy) / windows.back().newbounds.span_y() * windows.back().newbounds.sy);
  float real_sx = in_sx - ((in_ey - in_sy) / windows.back().newbounds.span_y() * windows.back().newbounds.sx);
  float real_ey = real_sy + (in_ey - in_sy) / windows.back().newbounds.span_y();
  
  /*
  dprintf("Zoom - input was %f,%f,%f, converted to %f,%f,%f\n", in_sx, in_sy, in_ey, real_sx, real_sy, real_ey);
  dprintf("aspect is %f\n", getAspect());
  dprintf("%f, %f\n", windows.back().newbounds.span_x(), windows.back().newbounds.span_y());
  dprintf("%f, %f, %f, %f\n", windows.back().newbounds.sx, windows.back().newbounds.sy, windows.back().newbounds.ex, windows.back().newbounds.ey);
  dprintf("%d\n", windows.size());*/
  
  map_sx = real_sx;
  map_sy = real_sy;
  map_zoom = real_ey - real_sy;
  map_ey = map_sy + map_zoom;
  map_ex = map_sx + map_zoom * windows.back().newbounds.span_x() / windows.back().newbounds.span_y();
}

Float4 getZoom() { return map_bounds; };
float getAspect() { return windows.back().newbounds.span_x() / windows.back().newbounds.span_y(); };

void setColor(float r, float g, float b) {
  r *= windows.back().fade;
  g *= windows.back().fade;
  b *= windows.back().fade;
  glColor3f(r, g, b);
  curcolor = Color(r, g, b);
}

void setColor(const Color &color) {
  setColor(color.r, color.g, color.b);
}

inline void localVertex2f(float x, float y) {
  glVertex2f((x - map_sx) / map_zoom, (y - map_sy) / map_zoom);
}

void drawLine(float sx, float sy, float ex, float ey, float weight) {
  CHECK(weight > 0);
  if(weight != curWeight || lineCount > 1000) {
    finishLineCluster();
    beginLineCluster(weight);
  }
  if(lastPoint != Float2(sx, sy)) {
    glColor3f(0, 0, 0);
    localVertex2f(lastPoint.x, lastPoint.y);
    localVertex2f(sx, sy);
    glColor3f(curcolor.r, curcolor.g, curcolor.b);
    localVertex2f(sx, sy);
    lpFailure++;
  } else {
    lpSuccess++;
  }
  localVertex2f(ex, ey);
  lastPoint = Float2(ex, ey);
  lineCount++;
  totalLineCount++;
}
void drawLine(const Float2 &s, const Float2 &e, float weight) {
  drawLine(s.x, s.y, e.x, e.y, weight);
}
void drawLine(const Coord2 &s, const Coord2 &e, float weight) {
  drawLine(s.toFloat(), e.toFloat(), weight);
}
void drawLine(const Float4 &pos, float weight) {
  drawLine(pos.sx, pos.sy, pos.ex, pos.ey, weight);
}
void drawLine(const Coord4 &loc, float weight) {
  drawLine(loc.toFloat(), weight);
}

void drawSolid(const Float4 &box) {
  PoolObj<vector<Float2> > bochs;
  bochs->push_back(Float2(box.sx, box.sy));
  bochs->push_back(Float2(box.sx, box.ey));
  bochs->push_back(Float2(box.ex, box.ey));
  bochs->push_back(Float2(box.ex, box.sy));
  drawSolidLoop(*bochs);
}

void drawSolidLoopWorker(const vector<Float2> &verts) {
  glBegin(GL_TRIANGLE_FAN);
  for(int i = 0; i < verts.size(); i++)
    localVertex2f(verts[i].x, verts[i].y);
  glEnd();
}

void drawSolidLoop(const vector<Float2> &verts) {
  for(int i = 0; i < verts.size(); i++)
    CHECK(inPath((verts[i] + verts[(i + 1) % verts.size()] + verts[(i + 2) % verts.size()]) / 3, verts));
  
  finishLineCluster();
  CHECK(glGetError() == GL_NO_ERROR);
  glColor3f(clearcolor.r, clearcolor.g, clearcolor.b);
  glBlendFunc(GL_ONE, GL_ZERO);
  drawSolidLoopWorker(verts);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  CHECK(glGetError() == GL_NO_ERROR);
  setColor(curcolor);
}

void invertStencilLoop(const vector<Coord2> &verts) {
  CHECK(inverting);
  
  glBegin(GL_TRIANGLE_FAN);
  for(int i = 0; i < verts.size(); i++)
    localVertex2f(verts[i].x.toFloat(), verts[i].y.toFloat());
  glEnd();
}

void drawPoint(const Float2 &pos, float weight) {
  finishLineCluster();
  glPointSize(weight / map_zoom * getResolutionY());   // GL uses pixels internally for this unit, so I have to translate from game-meters
  glBegin(GL_POINTS);
  glVertex2f((pos.x - map_sx) / map_zoom, (pos.y - map_sy) / map_zoom);
  glEnd();
}

/*************
 * Simple objects
 */

void drawLinePath(const vector<Float2> &verts, float weight) {
  CHECK(verts.size() >= 1);
  for(int i = 0; i < verts.size() - 1; i++)
    drawLine(verts[i], verts[i + 1], weight);
};
void drawLinePath(const vector<Coord2> &verts, float weight) {
  CHECK(verts.size() >= 1);
  for(int i = 0; i < verts.size() - 1; i++)
    drawLine(verts[i], verts[i + 1], weight);
};
void drawLineLoop(const vector<Float2> &verts, float weight) {
  CHECK(verts.size() >= 1);
  for(int i = 0; i < verts.size(); i++)
    drawLine(verts[i], verts[(i + 1) % verts.size()], weight);
};
void drawLineLoop(const vector<Coord2> &verts, float weight) {
  CHECK(verts.size() >= 1);
  for(int i = 0; i < verts.size(); i++)
    drawLine(verts[i], verts[(i + 1) % verts.size()], weight);
};

void drawTransformedLinePath(const vector<Float2> &verts, float angle, Float2 transform, float weight) {
  PoolObj<vector<Float2> > transed;
  for(int i = 0; i < verts.size(); i++)
    transed->push_back(rotate(verts[i], angle) + transform);
  drawLineLoop(*transed, weight);
};

void drawRect(const Float4 &rect, float weight) {
  PoolObj<vector<Float2> > verts;
  verts->push_back(Float2(rect.sx, rect.sy));
  verts->push_back(Float2(rect.sx, rect.ey));
  verts->push_back(Float2(rect.ex, rect.ey));
  verts->push_back(Float2(rect.ex, rect.sy));
  drawLineLoop(*verts, weight);
}

void drawRectAround(const Float2 &pos, float rad, float weight) {
  drawRect(Float4(pos.x - rad, pos.y - rad, pos.x + rad, pos.y + rad), weight);
}

void drawShadedRect(const Float4 &locs, float weight, float shadedens) {
  drawRect(locs, weight);
  float sp = locs.sx - locs.ey + locs.sy;
  sp = sp - fmod(sp, shadedens) + shadedens;
  for(float xp = sp; xp < locs.ex; xp += shadedens) {
    Float2 spos;
    Float2 epos;
    if(xp >= locs.sx) {
      spos = Float2(xp, locs.sy);
    } else {
      spos = Float2(locs.sx, locs.sx - xp + locs.sy);
    }
    if(xp + locs.ey - locs.sy < locs.ex) {
      epos = Float2(xp + locs.ey - locs.sy, locs.ey);
    } else {
      epos = Float2(locs.ex, locs.ex - xp + locs.sy);
    }
    drawLine(spos, epos, weight);
  }
}

void drawCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints, float weight) {
  PoolObj<vector<Float2> > verts;
  for(int i = 0; i <= midpoints; i++)
    verts->push_back(Float2(bezinterp(ptah.sx, ptah.ex, ptbh.sx, ptbh.ex, i / (float)midpoints), bezinterp(ptah.sy, ptah.ey, ptbh.sy, ptbh.ey, i / (float)midpoints)));
  drawLinePath(*verts, weight);
}

void drawCircle(const Float2 &center, float radius, float weight) {
  PoolObj<vector<Float2> > verts;
  for(int i = 0; i < 64; i++)
    verts->push_back(makeAngle(i * PI / 8) * radius + center);
  drawLineLoop(*verts, weight);
}

/*************
 * Text operations
 */

const int betweenletter = 1;
const float thickness = 0.5;

void drawText(const string &txt, float scale, float sx, float sy) {
  drawText(txt, scale, Float2(sx, sy));
}

void drawText(const string &txt, float scale, const Float2 &pos) {
  float sx = pos.x;
  scale /= 9;
  for(int i = 0; i < txt.size(); i++) {
    char kar = txt[i];
    if(!fontdata.count(kar)) {
      dprintf("Can't find font for character \"%c\"", kar);
      CHECK(0);
    }
    const FontCharacter &pathdat = fontdata[kar];
    for(int i = 0; i < pathdat.art.size(); i++) {
      PoolObj<vector<Float2> > verts;
      for(int j = 0; j < pathdat.art[i].size(); j++)
        verts->push_back(Float2(sx + pathdat.art[i][j].first * scale, pos.y + pathdat.art[i][j].second * scale));
      drawLinePath(*verts, scale * thickness);
    }
    sx += scale * (pathdat.width + betweenletter);
  }
}

float getTextWidth(const string &txt, float scale) {
  if(txt.size() == 0)
    return 0;
  const float point = scale / 9;
  float acum = point * (txt.size() - 1) * betweenletter;
  for(int i = 0; i < txt.size(); i++) {
    if(!fontdata.count(txt[i])) {
      dprintf("Couldn't find character '%c'", txt[i]);
      CHECK(0);
    }
    acum += point * fontdata[txt[i]].width;
  }
  return acum;
}

int snatchLine(const vector<string> &left, float fontsize, float limit) {
  for(int i = 0; i <= left.size(); i++) {
    string v;
    for(int k = 0; k < i; k++) {
      if(k)
        v += " ";
      v += left[k];
    }
    if(getTextWidth(v, fontsize) > limit)
      return i - 1;
  }
  return left.size();
}

vector<string> formatText(const string &txt, float fontsize, float width, const string &lineprefix) {
  vector<string> vlines;
  vector<string> paras = tokenize(txt, "\n");
  for(int i = 0; i < paras.size(); i++) {
    vector<string> left = tokenize(paras[i], " ");
    left[0] = lineprefix + left[0];
    while(left.size()) {
      int wordsa = snatchLine(left, fontsize, width);
      CHECK(wordsa > 0 && wordsa <= left.size());
      string v;
      for(int k = 0; k < wordsa; k++) {
        if(k)
          v += " ";
        v += left[k];
      }
      CHECK(getTextWidth(v, fontsize) <= width);
      vlines.push_back(v);
      left.erase(left.begin(), left.begin() + wordsa);
    }
  }
  return vlines;
}

float getFormattedTextHeight(const string &txt, float fontsize, float width) {
  const float linesize = fontsize * 1.5;
  const vector<string> text = formatText(txt, fontsize, width, "");
  if(text.size() == 0)
    return 0;
  return (text.size() - 1) * linesize + fontsize;
}

float getTextBoxBorder(float scale) { return scale / 2; }
float getTextBoxThickness(float scale) { return getTextBoxBorder(scale) / 5; }

void drawTextBoxAround(const Float4 &bounds, float textscale) {
  float gtbb = getTextBoxBorder(textscale);
  ColorStack csc(C::box_border);
  drawSolid(Float4(bounds.sx - gtbb, bounds.sy - gtbb, bounds.ex + gtbb, bounds.ey + gtbb));
  drawRect(Float4(bounds.sx - gtbb, bounds.sy - gtbb, bounds.ex + gtbb, bounds.ey + gtbb), getTextBoxThickness(textscale));
}

void drawJustifiedText(const string &txt, float scale, Float2 pos, int xps, int yps) {
  float wid = getTextWidth(txt, scale);
  if(xps == TEXT_MIN) {
  } else if(xps == TEXT_CENTER) {
    pos.x -= wid / 2;
  } else if(xps == TEXT_MAX) {
    pos.x -= wid;
  } else {
    CHECK(0);
  }
  
  if(yps == TEXT_MIN) {
  } else if(yps == TEXT_CENTER) {
    pos.y -= scale / 2;
  } else if(yps == TEXT_MAX) {
    pos.y -= scale;
  } else {
    CHECK(0);
  }
  
  drawText(txt, scale, pos);
}

void drawJustifiedMultiText(const vector<string> &txt, float letterscale, Float2 pos, int xps, int yps) {
  float gapscale = letterscale / 2;
  float hei = txt.size() * letterscale + (txt.size() - 1) * gapscale;
  if(yps == TEXT_MIN) {
  } else if(yps == TEXT_CENTER) {
    pos.y -= hei / 2;
  } else if(yps == TEXT_MAX) {
    pos.y -= hei;
  }
  
  for(int i = 0; i < txt.size(); i++) {
    drawJustifiedText(txt[i], letterscale, pos, xps, TEXT_MIN);
    pos.y += letterscale + gapscale;
  }
}

void drawFormattedText(const string &txt, float scale, Float4 bounds) {
  CHECK(!count(txt.begin(), txt.end(), '\n'));
  const vector<string> lines = formatText(txt, scale, bounds.span_x(), "");
  drawJustifiedMultiText(lines, scale, Float2(bounds.sx, bounds.sy), TEXT_MIN, TEXT_MIN);
}

void drawParagraphedText(const string &txt, float scale, pair<float, float> x_bounds, float y) {
  drawJustifiedParagraphedText(txt, scale, x_bounds, y, TEXT_MIN);
}

void drawJustifiedParagraphedText(const string &txt, float scale, pair<float, float> x_bounds, float y, int just) {
  const vector<string> lines = formatText(txt, scale, x_bounds.second - x_bounds.first, "  ");
  drawJustifiedMultiText(lines, scale, Float2(x_bounds.first, y), TEXT_MIN, just);
}


/*************
 * Vector path operations
 */

void drawVectorPath(const VectorPath &vecob, const pair<Float2, float> &coord, int midpoints, float weight) {
  for(int i = 0; i < vecob.vpath.size(); i++) {
    int j = (i + 1) % vecob.vpath.size();
    CHECK(vecob.vpath[i].curvr == vecob.vpath[j].curvl);
    Float2 l = vecob.center + vecob.vpath[i].pos;
    Float2 r = vecob.center + vecob.vpath[j].pos;
    
    // these are invalid and meaningless if it's not a curve, but hey!
    Float2 lc = l + vecob.vpath[i].curvrp;
    Float2 rc = r + vecob.vpath[j].curvlp;
    
    l = l * coord.second + coord.first;
    r = r * coord.second + coord.first;
    
    lc = lc * coord.second + coord.first;
    rc = rc * coord.second + coord.first;
    
    if(vecob.vpath[i].curvr) {
      drawCurve(Float4(l, lc), Float4(rc, r), midpoints, weight);
    } else {
      drawLine(Float4(l, r), weight);
    }
  }
}

void drawVectorPath(const VectorPath &vecob, const Float4 &bounds, int midpoints, float weight) {
  CHECK(bounds.isNormalized());
  drawVectorPath(vecob, fitInside(bounds, vecob.boundingBox()), midpoints, weight);
}

void drawDvec2(const Dvec2 &vecob, const Float4 &bounds, int midpoints, float weight) {
  CHECK(vecob.entities.size() == 0);
  pair<Float2, float> dimens = fitInside(vecob.boundingBox(), bounds);
  dimens.second *= vecob.scale;
  //dprintf("fit %f,%f,%f,%f into %f,%f,%f,%f, got %f,%f, %f\n", vecob.boundingBox().sx, vecob.boundingBox().sy,
      //vecob.boundingBox().ex, vecob.boundingBox().ey, bounds.sx, bounds.sy, bounds.ex, bounds.ey,
      //dimens.first.first, dimens.first.second, dimens.second);
  for(int i = 0; i < vecob.paths.size(); i++)
    drawVectorPath(vecob.paths[i], dimens, midpoints, weight);
}

/*************
 * Miscellaneous operations
 */

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight) {
  for(int i = 0; i < dupes; i++) {
    float ang = ((float)numer / denom + (float)i / dupes) * 2 * PI;
    drawLine(x, y, x + cos(ang) * len, y + sin(ang) * len, weight);
  }
}

static float roundUpGrid(float val, float spacing) {
  CHECK(spacing > 0);
  return ceil(val / spacing) * spacing;
};

void drawGrid(float spacing, float size) {
  for(float s = roundUpGrid(map_sx, spacing); s < roundUpGrid(map_ex, spacing); s += spacing)
    drawLine(s, map_sy, s, map_ey, size);
  for(float s = roundUpGrid(map_sy, spacing); s < roundUpGrid(map_ey, spacing); s += spacing)
    drawLine(map_sx, s, map_ex, s, size);
}

void drawCrosshair(const CFC2 &pos, float rad, float weight) {
  drawLine(pos->x - rad, pos->y, pos->x + rad, pos->y, weight);
  drawLine(pos->x, pos->y - rad, pos->x, pos->y + rad, weight);
}

void drawBlast(const Float2 &center, float rad, float chaos, int vertices) {
  const float ofs = unsync().frand() * 2 * PI / vertices;
  vector<Float2> pex;
  for(int j = 0; j < vertices; j++)
    pex.push_back(center + makeAngle(j * PI * 2 / vertices + ofs) * rad + Float2(unsync().gaussian(), unsync().gaussian()) * chaos);
  drawLineLoop(pex, 0.3);
}

vector<Color> cstack;

ColorStack::ColorStack(const Color &color) {
  cstack.push_back(curcolor);
  setColor(color);
}
ColorStack::~ColorStack() {
  setColor(cstack.back());
  cstack.pop_back();
}
