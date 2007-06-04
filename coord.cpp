
#include "coord.h"

#include "composite-imp.h"
#include "float.h"
#include "util.h"
#include "const.h"

using namespace std;

class Coords {
public:
  typedef Coord T1;
  typedef Coord2 T2;
  typedef Coord4 T4;
  
  static Coord atan2(Coord a, Coord b) { return Coord(float(::atan2(a.toFloat(), b.toFloat()))); }
  static Coord sin(Coord a) { return cfsin(a); };
  static Coord cos(Coord a) { return cfcos(a); };
  
  static const Coord tPI;
};
const Coord Coords::tPI = COORDPI;

string Coord::rawstr() const {
  return StringPrintf("%08x%08x", (unsigned int)(d >> 32), (unsigned int)d);
}

Coord coordFromRawstr(const string &lhs) {
  if(lhs.size() != 16) {
    dprintf("%s\n", lhs.c_str());
    CHECK(lhs.size() == 16);
  }
  for(int i = 0; i < lhs.size(); i++)
    CHECK(isdigit(lhs[i]) || (lhs[i] >= 'a' && lhs[i] <= 'f'));
  long long dd = 0;
  for(int i = 0; i < 16; i++) {
    dd *= 16;
    if(isdigit(lhs[i]))
      dd += lhs[i] - '0';
    else
      dd += lhs[i] - 'a' + 10;
  }
  CHECK(coordExplicit(dd).rawstr() == lhs);
  return coordExplicit(dd);
}

Float2 Coord2::toFloat() const { return Float2(x.toFloat(), y.toFloat()); }
Coord2::Coord2(const Float2 &rhs) : x(rhs.x), y(rhs.y) { };
string Coord2::rawstr() const {
  return StringPrintf("%s %s", x.rawstr().c_str(), y.rawstr().c_str());
}

Float4 Coord4::toFloat() const { return Float4(sx.toFloat(), sy.toFloat(), ex.toFloat(), ey.toFloat()); }
string Coord4::rawstr() const {
  return StringPrintf("%s %s %s %s", sx.rawstr().c_str(), sy.rawstr().c_str(), ex.rawstr().c_str(), ey.rawstr().c_str());
}

/*************
 * Computational geometry
 */

Coord len(const Coord2 &in) { return Coord(len(in.toFloat())); };
Coord2 normalize(const Coord2 &in) { return imp_normalize<Coords>(in); };

Coord getAngle(const Coord2 &in) { return imp_getAngle<Coords>(in); };
Coord2 makeAngle(const Coord &in) { return imp_makeAngle<Coords>(in); };

int whichSide(const Coord4 &f4, const Coord2 &pta) { return imp_whichSide<Coords>(f4, pta); };

Coord distanceFromLine(const Coord4 &line, const Coord2 &pt) {
  Coord u = ((pt.x - line.sx) * (line.ex - line.sx) + (pt.y - line.sy) * (line.ey - line.sy)) / ((line.ex - line.sx) * (line.ex - line.sx) + (line.ey - line.sy) * (line.ey - line.sy));
  if(u < 0 || u > 1)
    return min(len(pt - Coord2(line.sx, line.sy)), len(pt - Coord2(line.ex, line.ey)));
  Coord2 ipt = Coord2(line.sx, line.sy) + Coord2(line.ex - line.sx, line.ey - line.sy) * u;
  return len(ipt - pt);
}

int inPath(const Coord2 &point, const vector<Coord2> &path) {
  return imp_inPath<Coords>(point, path);
};

bool roughInPath(const Coord2 &point, const vector<Coord2> &path, int goal) {
  int dx[] = {0, 0, 0, 1, -1, 1, 1, -1, -1};
  int dy[] = {0, 1, -1, 0, 0, 1, -1, 1, -1};
  for(int i = 0; i < 9; i++)
    if((bool)inPath(point + Coord2(dx[i], dy[i]) / 65536, path) == goal)
      return true;
  return false;
}

Coord2 getPointIn(const vector<Coord2> &path) {
  // TODO: find a point inside the polygon in a better fashion
  //GetDifferenceHandler CrashHandler(path, path);
  Coord2 pt;
  bool found = false;
  for(int j = 0; j < path.size() && !found; j++) {
    Coord2 pospt = (path[j] + path[(j + 1) % path.size()] + path[(j + 2) % path.size()]) / 3;
    if(inPath(pospt, path)) {
      pt = pospt;
      found = true;
    }
  }
  CHECK(found);
  return pt;
}

bool pathReversed(const vector<Coord2> &path) {
  return imp_pathReversed<Coords>(path);
}

int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs) {
  for(int i = 0; i < lhs.size(); i++) {
    int i2 = (i + 1) % lhs.size();
    for(int j = 0; j < rhs.size(); j++) {
      int j2 = (j + 1) % rhs.size();
      if(linelineintersect(Coord4(lhs[i], lhs[i2]), Coord4(rhs[j], rhs[j2])))
        return PR_INTERSECT;
    }
  }
  bool lir = inPath(getPointIn(lhs), rhs);
  bool ril = inPath(getPointIn(rhs), lhs);
  if(!lir && !ril) {
    return PR_SEPARATE;
  } else if(lir && !ril) {
    return PR_RHSENCLOSE;
  } else if(!lir && ril) {
    return PR_LHSENCLOSE;
  } else if(lir && ril && abs(getArea(lhs)) < abs(getArea(rhs))) {
    return PR_RHSENCLOSE;
  } else if(lir && ril && abs(getArea(lhs)) > abs(getArea(rhs))) {
    return PR_LHSENCLOSE;
  } else {
    // dammit, don't send the same two paths! we deny!
    CHECK(0);
  }
}

Coord getArea(const vector<Coord2> &are) { return imp_getArea<Coords>(are); }
Coord2 getCentroid(const vector<Coord2> &are) { return imp_getCentroid<Coords>(are); }
Coord getPerimeter(const vector<Coord2> &are) {
  Coord totperi = 0;
  for(int i = 0; i < are.size(); i++) {
    int j = (i + 1) % are.size();
    totperi += len(are[i] - are[j]);
  }
  return totperi;
}
Coord4 getBoundBox(const vector<Coord2> &are) {
  Coord4 bbox = startCBoundBox();
  for(int i = 0; i < are.size(); i++)
    addToBoundBox(&bbox, are[i]);
  CHECK(bbox.isNormalized());
  return bbox;
}

bool colinear(const Coord4 &line, const Coord2 &pt) {
  Coord koord = distanceFromLine(line, pt);
  return koord < Coord(0.00001f);
}

/*************
 * Bounding box
 */

Coord4 startCBoundBox() { return imp_startBoundBox<Coords>(); };

void addToBoundBox(Coord4 *bbox, const Coord4 &rect) { return imp_addToBoundBox<Coords>(bbox, rect); };
void addToBoundBox(Coord4 *bbox, const vector<Coord2> &line) { return imp_addToBoundBox<Coords>(bbox, line); };

void expandBoundBox(Coord4 *bbox, Coord factor) { return imp_expandBoundBox<Coords>(bbox, factor); };

/*************
 * Math
 */

bool linelineintersect(const Coord4 &lhs, const Coord4 &rhs) { return imp_linelineintersect<Coords>(lhs, rhs); };
Coord linelineintersectpos(const Coord4 &lhs, const Coord4 &rhs) { return imp_linelineintersectpos<Coords>(lhs, rhs); };

Coord2 lerp(const Coord2 &lhs, const Coord2 &rhs, Coord dist) {
  return lhs + (rhs - lhs) * dist;
}

Coord4 lerp(const Coord4 &lhs, const Coord4 &rhs, Coord dist) {
  return lhs + (rhs - lhs) * dist;
}
