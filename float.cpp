
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

Float2 reflect(const Float2 &incoming, float normal) {
  if(len(incoming) == 0)
    return Float2(0, 0);
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


/*************
 * Matrixtastic
 */

void Transform2d::hflip() {
  for(int i = 0; i < 3; i++)
    m[0][i] *= -1;
}
void Transform2d::vflip() {
  for(int i = 0; i < 3; i++)
    m[1][i] *= -1;
}
void Transform2d::dflip() {
  for(int i = 0; i < 3; i++)
    swap(m[0][i], m[1][i]);
}

float Transform2d::det() {
  float rv = 0;
  for(int x = 0; x < 3; x++) {
    float tv = m[x][0] * detchunk(x, 0);
    if(x % 2)
      tv = -tv;
    rv += tv;
  }
  return rv;
}

vector<int> allExcept(int t) {
  CHECK(t >= 0 && t < 3);
  vector<int> rv;
  for(int i = 0; i < 3; i++)
    rv.push_back(i);
  rv.erase(find(rv.begin(), rv.end(), t));
  CHECK(rv.size() == 2);
  return rv;
}

float Transform2d::detchunk(int x, int y) {
  // this code sucks.
  vector<int> xv = allExcept(x);
  vector<int> yv = allExcept(y);
  CHECK(xv.size() == 2 && yv.size() == 2);
  return m[xv[0]][yv[0]] * m[xv[1]][yv[1]] - m[xv[0]][yv[1]] * m[xv[1]][yv[0]];
}

void Transform2d::invert() {
  // hahahahahhahah.
  Transform2d res;
  for(int x = 0; x < 3; x++) {
    for(int y = 0; y < 3; y++) {
      res.m[x][y] = detchunk(y, x) / det();
      if((x + y) % 2)
        res.m[x][y] = -res.m[x][y];
    }
  }
  *this = res;
}

void Transform2d::transform(Float2 *pos) const {
  *pos = Float2(m[0][0] * pos->x + m[0][1] * pos->y + m[0][2], m[1][0] * pos->x + m[1][1] * pos->y + m[1][2]);
}

void Transform2d::display() const {
  for(int i = 0; i < 3; i++)
    dprintf("  %f %f %f\n", m[0][i], m[1][i], m[2][i]);
}

Transform2d::Transform2d() {
  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 3; j++)
      m[i][j] = (i == j);
}

Transform2d operator*(const Transform2d &lhs, const Transform2d &rhs) {
  Transform2d rv;
  for(int x = 0; x < 3; x++) {
    for(int y = 0; y < 3; y++) {
      rv.m[x][y] = 0;
      for(int z = 0; z < 3; z++) {
        rv.m[x][y] += lhs.m[z][y] * rhs.m[x][z];
      }
    }
  }
  return rv;
}

Transform2d &operator*=(Transform2d &lhs, const Transform2d &rhs) {
  lhs = lhs * rhs;
  return lhs;
}

Transform2d t2d_identity() {
  return Transform2d();
}
Transform2d t2d_flip(bool h, bool v, bool d) {
  Transform2d o;
  if(h)
    o.hflip();
  if(v)
    o.vflip();
  if(d)
    o.dflip();
  return o;
}
Transform2d t2d_rotate(float rads) {
  Transform2d o;
  o.m[0][0] = cos(rads);
  o.m[0][1] = sin(rads);
  o.m[1][0] = -sin(rads);
  o.m[1][1] = cos(rads);
  return o;
}
