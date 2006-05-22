
#include "gfx.h"

#include <cmath>
#include <fstream>
#include <map>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>

#include "util.h"
#include "parse.h"
#include "args.h"

DEFINE_int(resolution_x, -1, "X resolution (Y is X/4*3), -1 for autodetect");

class GfxWindowState {
public:
  float saved_sx;
  float saved_sy;
  float saved_ey;

  Float4 newbounds;

  GfxWindow *gfxw;
};
static vector<GfxWindowState> windows;

void setDefaultResolution(int width, int height, bool fullscreen) {
  dprintf("Current detected resolution: %d/%d\n", width, height);
  if(FLAGS_resolution_x == -1) {
    FLAGS_resolution_x = width;
    if(!fullscreen)
      resDown();
  }
}
  
int getResolutionX() {
  CHECK(FLAGS_resolution_x > 0);
  CHECK(FLAGS_resolution_x % 4 == 0);
  return FLAGS_resolution_x;
}
int getResolutionY() {
  CHECK(FLAGS_resolution_x > 0);
  CHECK(FLAGS_resolution_x % 4 == 0);
  return FLAGS_resolution_x / 4 * 3;
}

void resDown() {
  const int reses[] = { 1600, 1400, 1280, 1152, 1024, 800, 640, -1 };
  CHECK(FLAGS_resolution_x > 0);
  for(int i = 0; i < sizeof(reses) / sizeof(*reses); i++) {
    if(FLAGS_resolution_x > reses[i]) {
      FLAGS_resolution_x = reses[i];
      break;
    }
  }
  CHECK(FLAGS_resolution_x > 0);
}

Color::Color() { };
Color::Color(float in_r, float in_g, float in_b) :
  r(in_r), g(in_g), b(in_b) { };

Color colorFromString(const string &str) {
  if(str.size() == 3) {
    // 08f style color
    return Color(fromHex(str[0]) / 15., fromHex(str[1]) / 15., fromHex(str[2]) / 15.);
  } else {
    // 1.0, 0.3, 0.194 style color
    vector<string> toki = tokenize(str, " ,");
    CHECK(toki.size() == 3);
    return Color(atof(toki[0].c_str()), atof(toki[1].c_str()), atof(toki[2].c_str()));
  }
}

Color operator+(const Color &lhs, const Color &rhs) {
  return Color(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
}
Color operator*(const Color &lhs, float rhs) {
  return Color(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
}
Color operator/(const Color &lhs, float rhs) {
  return Color(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs);
}

const Color &operator+=(Color &lhs, const Color &rhs) {
  lhs = lhs + rhs;
  return lhs;
}

static float map_sx;
static float map_sy;
static float map_ex;
static float map_ey;
static float map_zoom;

static float map_saved_sx;
static float map_saved_sy;
static float map_saved_ey;

static map< char, vector< vector< pair< int, int > > > > fontdata;

void initGfx() {
  {
    ifstream font("data/font.txt");
    string line;
    CHECK(font);
    while(getLineStripped(font, &line)) {
      dprintf("Parsing font character \"%s\"\n", line.c_str());
      vector<string> first = tokenize(line, ":");
      if(first.size() == 1)
        first.push_back("");
      CHECK(first.size() == 2);
      if(first.size() == 2) {
        if(first[0] == "colon")
          first[0] = ":";
        if(first[0] == "hash")
          first[0] = "#";
        if(first[0] == "space")
          first[0] = " ";
        CHECK(first[0].size() == 1);
        vector< string > paths = tokenize(first[1], "|");
        CHECK(!fontdata.count(first[0][0]));
        fontdata[first[0][0]]; // creates it
        for(int i = 0; i < paths.size(); i++) {
          vector< string > order = tokenize(paths[i], " ");
          CHECK(order.size() != 1);
          vector< pair< int, int > > tpath;
          for(int j = 0; j < order.size(); j++) {
            vector< int > out = sti(tokenize(order[j], ","));
            CHECK(out.size() == 2);
            tpath.push_back(make_pair(out[0], out[1]));
          }
          if(tpath.size()) {
            CHECK(tpath.size() >= 2);
            fontdata[first[0][0]].push_back(tpath);
          }
        }
      }
    }
  }
  {
    GLfloat flipy[16]= { (5.0/4.0) / (4.0/3.0), 0, 0, 0,   0, -1, 0, 0,   0, 0, 1, 0,  0, 0, 0, 1 };
    glMultMatrixf(flipy);
    glTranslatef(0, -1, 0);
  }
  {
    GfxWindowState gfws;
    gfws.saved_sx = 0;
    gfws.saved_sy = 0;
    gfws.saved_ey = 1;

    gfws.newbounds = Float4(0, 0, 4.0/3.0, 1);
    gfws.gfxw = NULL;
    
    windows.push_back(gfws);
  }
}

float curWeight = -1.f;
int lineCount = 0;
int clusterCount = 0;
int lpFailure = 0;
int lpSuccess = 0;
int lastStats = 0;

static Color curcolor;
static Color clearcolor;
const Float2 invPoint(-1234e12, 394e9);
Float2 lastPoint = invPoint;

string printGraphicsStats() {
  string gstat = StringPrintf("%f average clusters over %d frames, %d/%d strip hit (%.2f%%)", clusterCount / float(frameNumber - lastStats), frameNumber - lastStats, lpSuccess, lpFailure + lpSuccess, lpSuccess / float(lpFailure + lpSuccess) * 100);
  clusterCount = 0;
  lpFailure = 0;
  lpSuccess = 0;
  lastStats = frameNumber;
  return gstat;
}

void beginLineCluster(float weight) {
  CHECK(glGetError() == GL_NO_ERROR);
  CHECK(curWeight == -1.f);
  CHECK(weight != -1.f);
  glLineWidth( weight / map_zoom * getResolutionY() );   // GL uses pixels internally for this unit, so I have to translate from game-meters
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

void initFrame() {
  CHECK(curWeight == -1.f);
  glEnable( GL_POINT_SMOOTH );
  glEnable( GL_LINE_SMOOTH );
  glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
  glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
  //glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );
  //glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE );
  //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  clearFrame(Color(0.1, 0.1, 0.1));
  CHECK(glGetError() == GL_NO_ERROR);
}

void clearFrame(const Color &color) {
  finishLineCluster();
  glClearColor( color.r, color.g, color.b, 0.0f );
  glClear(GL_COLOR_BUFFER_BIT);
  clearcolor = color;
}

void deinitFrame() {
  finishLineCluster();
  glFlush();
  SDL_GL_SwapBuffers(); 
  CHECK(glGetError() == GL_NO_ERROR);
}

GfxWindow::GfxWindow(const Float4 &bounds) {
  GfxWindowState gfws;
  gfws.saved_sx = map_saved_sx;
  gfws.saved_sy = map_saved_sy;
  gfws.saved_ey = map_saved_ey;

  // newbounds is the location on the screen that the new bounds fill
  gfws.newbounds = Float4((bounds.sx - map_sx) / map_zoom, (bounds.sy - map_sy) / map_zoom, (bounds.ex - map_sx) / map_zoom, (bounds.ey - map_sy) / map_zoom);
  
  gfws.gfxw = this;
  
  windows.push_back(gfws);
  
  // We should also be making the opengl window call here, but for now I am lazy
}
GfxWindow::~GfxWindow() {
  CHECK(windows.size() > 1);
  CHECK(windows.back().gfxw == this);
  float tmap_sx = windows.back().saved_sx;
  float tmap_sy = windows.back().saved_sy;
  float tmap_ey = windows.back().saved_ey;
  windows.pop_back();
  setZoom(tmap_sx, tmap_sy, tmap_ey);
}

void setZoom(float in_sx, float in_sy, float in_ey) {
  finishLineCluster();
  
  map_saved_sx = in_sx;
  map_saved_sy = in_sy;
  map_saved_ey = in_ey;
  
  float real_sy = in_sy - ((in_ey - in_sy) / windows.back().newbounds.y_span() * windows.back().newbounds.sy);
  float real_sx = in_sx - ((in_ey - in_sy) / windows.back().newbounds.y_span() * windows.back().newbounds.sx);
  float real_ey = real_sy + (in_ey - in_sy) / windows.back().newbounds.y_span();
  
  /*
  dprintf("Zoom - input was %f,%f,%f, converted to %f,%f,%f\n", in_sx, in_sy, in_ey, real_sx, real_sy, real_ey);
  dprintf("aspect is %f\n", getAspect());
  dprintf("%f, %f\n", windows.back().newbounds.x_span(), windows.back().newbounds.y_span());
  dprintf("%f, %f, %f, %f\n", windows.back().newbounds.sx, windows.back().newbounds.sy, windows.back().newbounds.ex, windows.back().newbounds.ey);
  dprintf("%d\n", windows.size());*/
  
  map_sx = real_sx;
  map_sy = real_sy;
  map_zoom = real_ey - real_sy;
  map_ey = map_sy + map_zoom;
  map_ex = map_sx + map_zoom * 4 / 3;
}

void setZoomAround(const Coord4 &bbox) {
  Coord2 center = bbox.midpoint();
  Coord zoomtop = bbox.y_span() / 2;
  Coord zoomside = bbox.x_span() / 2;
  Coord zoomtopfinal = max(zoomtop, zoomside / (Coord)getAspect());
  setZoom((center.x - zoomtopfinal * (Coord)getAspect()).toFloat(), (center.y - zoomtopfinal).toFloat(), (center.y + zoomtopfinal).toFloat());
}

void setZoomCenter(float cx, float cy, float radius_y) {
  setZoom(cx - radius_y * getAspect(), cy - radius_y, cy + radius_y);
}

float getAspect() { return windows.back().newbounds.x_span() / windows.back().newbounds.y_span(); };

void setColor( float r, float g, float b ) {
  glColor3f( r, g, b );
  curcolor = Color(r, g, b);
}

void setColor(const Color &color) {
  setColor(color.r, color.g, color.b);
}

void localVertex2f(float x, float y) {
  glVertex2f( ( x - map_sx ) / map_zoom, ( y - map_sy ) / map_zoom );
}

void drawLine( float sx, float sy, float ex, float ey, float weight ) {
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
}
void drawLine(const Float2 &s, const Float2 &e, float weight) {
  drawLine(s.x, s.y, e.x, e.y, weight);
}
void drawLine(const Coord2 &s, const Coord2 &e, float weight) {
  drawLine(s.toFloat(), e.toFloat(), weight);
}
void drawLine( const Float4 &pos, float weight ) {
  drawLine( pos.sx, pos.sy, pos.ex, pos.ey, weight );
}
void drawLine( const Coord4 &loc, float weight ) {
  drawLine(loc.toFloat(), weight);
}

void drawLinePath(const vector<Float2> &verts, float weight) {
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
  for(int i = 0; i < verts.size(); i++) {
    drawLine(verts[i], verts[(i + 1) % verts.size()], weight);
    //drawLine(verts[i], (verts[i] + verts[(i + 1) % verts.size()]) / 2, weight * 2);
  }
};

void drawTransformedLinePath(const vector<Float2> &verts, float angle, Float2 transform, float weight) {
  vector<Float2> transed;
  for(int i = 0; i < verts.size(); i++)
    transed.push_back(rotate(verts[i], angle) + transform);
  drawLineLoop(transed, weight);
};

void drawSolid(const Float4 &box) {
  CHECK(box.isNormalized());
  finishLineCluster();
  CHECK(glGetError() == GL_NO_ERROR);
  glColor3f(clearcolor.r, clearcolor.g, clearcolor.b);
  glBlendFunc( GL_ONE, GL_ZERO );
  glBegin(GL_TRIANGLE_STRIP);
  localVertex2f(box.sx, box.sy);
  localVertex2f(box.sx, box.ey);
  localVertex2f(box.ex, box.sy);
  localVertex2f(box.ex, box.ey);
  glEnd();
  glBlendFunc( GL_SRC_ALPHA, GL_ONE );
  CHECK(glGetError() == GL_NO_ERROR);
  setColor(curcolor);
}

void drawRect( const Float4 &rect, float weight ) {
  vector<Float2> verts;
  verts.push_back(Float2(rect.sx, rect.sy));
  verts.push_back(Float2(rect.sx, rect.ey));
  verts.push_back(Float2(rect.ex, rect.ey));
  verts.push_back(Float2(rect.ex, rect.sy));
  drawLineLoop(verts, weight);
}

void drawRectAround(float x, float y, float rad, float weight) {
  drawRect(Float4(x - rad, y - rad, x + rad, y + rad), weight);
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

float bezinterp(float x0, float x1, float x2, float x3, float t) {
  float cx = 3 * ( x1 - x0 );
  float bx = 3 * ( x2 - x1 ) - cx;
  float ax = x3 - x0 - cx - bx;
  return ax * t * t * t + bx * t * t + cx * t + x0;
}

void drawCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints, float weight) {
  vector<Float2> verts;
  for(int i = 0; i <= midpoints; i++)
    verts.push_back(Float2(bezinterp(ptah.sx, ptah.ex, ptbh.sx, ptbh.ex, i / (float)midpoints), bezinterp(ptah.sy, ptah.ey, ptbh.sy, ptbh.ey, i / (float)midpoints)));
  drawLinePath(verts, weight);
}

void drawCurveControls(const Float4 &ptah, const Float4 &ptbh, float spacing, float weight) {
  drawRectAround( ptah.sx, ptah.sy, spacing, weight );
  drawRectAround( ptah.ex, ptah.ey, spacing, weight );
  drawRectAround( ptbh.sx, ptbh.sy, spacing, weight );
  drawRectAround( ptbh.ex, ptbh.ey, spacing, weight );
  drawLine( ptah, weight );
  drawLine( ptbh, weight );
}

void drawCircle(const Float2 &center, float radius, float weight) {
  vector<Float2> verts;
  for(int i = 0; i < 16; i++)
    verts.push_back(makeAngle(i * PI / 8) * radius + center);
  drawLineLoop(verts, weight);
}
void drawCircle(const Coord2 &center, Coord radius, Coord weight) {
  drawCircle(center.toFloat(), radius.toFloat(), weight.toFloat());
}

void drawPoint(float x, float y, float weight) {
  finishLineCluster();
  glPointSize( weight / map_zoom * getResolutionY() );   // GL uses pixels internally for this unit, so I have to translate from game-meters
  glBegin( GL_POINTS );
  glVertex2f( ( x - map_sx ) / map_zoom, ( y - map_sy ) / map_zoom );
  glEnd();
}

void drawText(const char *txt, float scale, float sx, float sy) {
  scale /= 9;
  for(int i = 0; txt[i]; i++) {
    char kar = toupper(txt[i]);
    if(!fontdata.count(kar)) {
      dprintf("Can't find font for character \"%c\"", kar);
      CHECK(0);
    }
    const vector<vector<pair<int, int> > > &pathdat = fontdata[kar];
    for(int i = 0; i < pathdat.size(); i++) {
      vector<Float2> verts;
      for(int j = 0; j < pathdat[i].size(); j++)
        verts.push_back(Float2(sx + pathdat[i][j].first * scale, sy + pathdat[i][j].second * scale));
      drawLinePath(verts, scale / 5);
    }
    sx += scale * 8;
  }
}

void drawText(const string &txt, float scale, float sx, float sy) {
  drawText(txt.c_str(), scale, sx, sy);
}

void drawText(const string &txt, float scale, const Float2 &pos) {
  drawText(txt.c_str(), scale, pos.x, pos.y);
}

void drawJustifiedText(const string &txt, float scale, float sx, float sy, int xps, int yps) {
  float lscale = scale / 9;
  
  float wid = lscale * ( 8 * txt.size() - 3 );
  if(xps == TEXT_MIN) {
  } else if(xps == TEXT_CENTER) {
    sx -= wid / 2;
  } else if(xps == TEXT_MAX) {
    sx -= wid;
  } else {
    CHECK(0);
  }
  
  if(yps == TEXT_MIN) {
  } else if(yps == TEXT_CENTER) {
    sy -= lscale * 9 / 2;
  } else if(yps == TEXT_MAX) {
    sy -= lscale * 9;
  } else {
    CHECK(0);
  }
  
  drawText(txt, scale, sx, sy);
}

void drawJustifiedMultiText(const vector<string> &txt, float letterscale, float gapscale, Float2 pos, int xps, int yps) {
  float hei = txt.size() * letterscale + (txt.size() - 1) * gapscale;
  if(yps == TEXT_MIN) {
  } else if(yps == TEXT_CENTER) {
    pos.y -= hei / 2;
  } else if(yps == TEXT_MAX) {
    pos.y -= hei;
  }
  
  for(int i = 0; i < txt.size(); i++) {
    drawJustifiedText(txt[i], letterscale, pos.x, pos.y, xps, TEXT_MIN);
    pos.y += letterscale + gapscale;
  }
}

void drawVectorPath(const VectorPath &vecob, const pair<Float2, float> &coord, int midpoints, float weight) {
  for(int i = 0; i < vecob.vpath.size(); i++) {
    int j = (i + 1) % vecob.vpath.size();
    CHECK(vecob.vpath[i].curvr == vecob.vpath[j].curvl);
    float lx = vecob.centerx + vecob.vpath[i].x;
    float ly = vecob.centery + vecob.vpath[i].y;
    float rx = vecob.centerx + vecob.vpath[j].x;
    float ry = vecob.centery + vecob.vpath[j].y;
    
    // these are invalid and meaningless if it's not a curve, but hey!
    float lcx = lx + vecob.vpath[i].curvrx;
    float lcy = ly + vecob.vpath[i].curvry;
    float rcx = rx + vecob.vpath[j].curvlx;
    float rcy = ry + vecob.vpath[j].curvly;
    
    lx *= coord.second;
    ly *= coord.second;
    rx *= coord.second;
    ry *= coord.second;
    
    lcx *= coord.second;
    lcy *= coord.second;
    rcx *= coord.second;
    rcy *= coord.second;
    
    lx += coord.first.x;
    ly += coord.first.y;
    rx += coord.first.x;
    ry += coord.first.y;
    
    lcx += coord.first.x;
    lcy += coord.first.y;
    rcx += coord.first.x;
    rcy += coord.first.y;
    if(vecob.vpath[i].curvr) {
      drawCurve(Float4(lx, ly, lcx, lcy), Float4(rcx, rcy, rx, ry), midpoints, weight);
    } else {
      drawLine(Float4(lx, ly, rx, ry), weight);
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
  //dprintf("fit %f,%f,%f,%f into %f,%f,%f,%f, got %f,%f, %f\n", vecob.boundingBox().sx, vecob.boundingBox().sy,
      //vecob.boundingBox().ex, vecob.boundingBox().ey, bounds.sx, bounds.sy, bounds.ex, bounds.ey,
      //dimens.first.first, dimens.first.second, dimens.second);
  for(int i = 0; i < vecob.paths.size(); i++)
    drawVectorPath(vecob.paths[i], dimens, midpoints, weight);
}

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight) {
  for(int i = 0; i < dupes; i++) {
    float ang = ((float)numer / denom + (float)i / dupes) * 2 * PI;
    drawLine(x, y, x + cos(ang) * len, y + sin(ang) * len, weight);
  }
}

void drawGrid(float spacing, float size) {
  CHECK(map_sx <= 0 && map_ex >= 0 && map_sy <= 0 && map_ey >= 0);
  for(float s = 0; s < map_ex; s += spacing)
    drawLine(s, map_sy, s, map_ey, size);
  for(float s = -spacing; s > map_sx; s -= spacing)
    drawLine(s, map_sy, s, map_ey, size);
  for(float s = 0; s < map_ey; s += spacing)
    drawLine(map_sx, s, map_ex, s, size);
  for(float s = -spacing; s > map_sy; s -= spacing)
    drawLine(map_sx, s, map_ex, s, size);
}

void drawCrosshair(float x, float y, float rad, float weight) {
  drawLine(x - rad, y, x + rad, y, weight);
  drawLine(x, y - rad, x, y + rad, weight);
}
