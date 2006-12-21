
#include "float.h"

#include "cfcommon.h"
#include "composite-imp.h"

using namespace std;

class Floats {
public:
  typedef float T1;
  typedef Float2 T2;
  typedef Float4 T4;
  
  static float atan2(float a, float b) { return ::atan2(a, b); }
  static float sin(float a) { return fsin(a); };
  static float cos(float a) { return fcos(a); };
  
  static const float tPI;
};
const float Floats::tPI = PI;

/*************
 * Computational geometry
 */

float len(const Float2 &in) { return imp_len<Floats>(in); }
Float2 normalize(const Float2 &in) { return imp_normalize<Floats>(in); }

float getAngle(const Float2 &in) { return imp_getAngle<Floats>(in); };
Float2 makeAngle(const float &in) { return imp_makeAngle<Floats>(in); };

int whichSide(const Float4 &f4, const Float2 &pta) { return imp_whichSide<Floats>(f4, pta); };

pair<Float2, float> fitInside(const Float4 &objbounds, const Float4 &goalbounds) { return imp_fitInside<Floats>(objbounds, goalbounds); }

Float2 getCentroid(const vector<Float2> &are) { return imp_getCentroid<Floats>(are); }

int inPath(const Float2 &point, const vector<Float2> &path) {
  return imp_inPath<Floats>(point, path);
};

bool pathReversed(const vector<Float2> &path) {
  return imp_pathReversed<Floats>(path);
}

float getArea(const vector<Float2> &are) {
  return imp_getArea<Floats>(are);
}
/*
Float2 reflect(const Float2 &incoming, float normal) {
  return makeAngle(normal) * len(incoming);
}*/

Float2 reflect(const Float2 &incoming, float normal) {
  return makeAngle(-(getAngle(-incoming) - normal) + normal) * len(incoming);
}

/*************
 * Bounding box
 */

Float4 startFBoundBox() { return imp_startBoundBox<Floats>(); };

void addToBoundBox(Float4 *bbox, float x, float y) { return imp_addToBoundBox<Floats>(bbox, x, y); };
void addToBoundBox(Float4 *bbox, const Float2 &point) { return imp_addToBoundBox<Floats>(bbox, point); };
void addToBoundBox(Float4 *bbox, const Float4 &rect) { return imp_addToBoundBox<Floats>(bbox, rect); };

void expandBoundBox(Float4 *bbox, float factor) { return imp_expandBoundBox<Floats>(bbox, factor); };

/*************
 * Math
 */

bool linelineintersect(const Float4 &lhs, const Float4 &rhs) { return imp_linelineintersect<Floats>(lhs, rhs); };
float linelineintersectpos(const Float4 &lhs, const Float4 &rhs) { return imp_linelineintersectpos<Floats>(lhs, rhs); };

Float2 rotate(const Float2 &in, float ang) {
  return makeAngle(getAngle(in) + ang) * len(in);
}

Float4 squareInside(const Float4 &in) {
  float minwid = min(in.ex - in.sx, in.ey - in.sy);
  return boxAround(in.midpoint(), minwid / 2);
};

Float4 extend(const Float4 &in, float amount) {
  return Float4(in.sx - amount, in.sy - amount, in.ex + amount, in.ey + amount);
};
Float4 contract(const Float4 &in, float amount) {
  return Float4(in.sx + amount, in.sy + amount, in.ex - amount, in.ey - amount);
};
