#ifndef DNET_FLOAT
#define DNET_FLOAT

#include "util.h"

#include <cmath>

using namespace std;

/*************
 * Float2/Float4 classes and operators
 */

class Float2 {
public:
  float x, y;
  Float2() { };
  Float2(float in_x, float in_y) :
    x(in_x), y(in_y) { };
};

inline Float2 operator-(const Float2 &lhs) {
  return Float2(-lhs.x, -lhs.y);
}

inline Float2 operator+(const Float2 &lhs, const Float2 &rhs) {
  return Float2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline Float2 operator-(const Float2 &lhs, const Float2 &rhs) {
  return Float2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline Float2 operator*(const Float2 &lhs, float rhs) {
  return Float2(lhs.x * rhs, lhs.y * rhs);
}

inline Float2 operator/(const Float2 &lhs, float rhs) {
  return Float2(lhs.x / rhs, lhs.y / rhs);
}

inline Float2 &operator+=(Float2 &lhs, const Float2 &rhs) {
  lhs = lhs + rhs;
  return lhs;
}

inline Float2 &operator-=(Float2 &lhs, const Float2 &rhs) {
  lhs = lhs - rhs;
  return lhs;
}

inline Float2 &operator*=(Float2 &lhs, float rhs) {
  lhs = lhs * rhs;
  return lhs;
}

inline Float2 &operator/=(Float2 &lhs, float rhs) {
  lhs = lhs / rhs;
  return lhs;
}

inline bool operator==(const Float2 &lhs, const Float2 &rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const Float2 &lhs, const Float2 &rhs) {
  return lhs.x != rhs.x || lhs.y != rhs.y;
}

// not meant to be meaningful
inline bool operator<(const Float2 &lhs, const Float2 &rhs) {
  return lhs.x < rhs.x || lhs.x == rhs.x && lhs.y < rhs.y;
}

class Float4 {
public:
  float sx, sy, ex, ey;

  inline Float4 normalize() const {
    Float4 cp = *this;
    float tmp;
    if(cp.sx > cp.ex) {
      tmp = cp.sx;
      cp.sx = cp.ex;
      cp.ex = tmp;
    }
    if(cp.sy > cp.ey) {
      tmp = cp.sy;
      cp.sy = cp.ey;
      cp.ey = tmp;
    }
    return cp;
  }

  bool isNormalized() const {
    return sx <= ex && sy <= ey;
  }
  
  Float2 s() const {
    return Float2(sx, sy);
  }
  Float2 e() const {
    return Float2(ex, ey);
  }
  
  pair<float, float> xs() const {
    return make_pair(sx, ex);
  }
  
  float span_x() const {
    return ex - sx;
  }
  float span_y() const {
    return ey - sy;
  }
  
  Float2 midpoint() const {
    return (s() + e()) / 2;
  }
  
  Float4() { };
  Float4(const Float2 &s, const Float2 &e) :
    sx(s.x), sy(s.y), ex(e.x), ey(e.y) { };
  Float4(float in_sx, float in_sy, float in_ex, float in_ey) :
    sx(in_sx), sy(in_sy), ex(in_ex), ey(in_ey) { };
};

inline Float4 operator+(const Float4 &lhs, const Float4 &rhs) {
  return Float4(lhs.sx + rhs.sx, lhs.sy + rhs.sy, lhs.ex + rhs.ex, lhs.ey + rhs.ey);
}
inline Float4 operator*(const Float4 &lhs, float rhs) {
  return Float4(lhs.sx * rhs, lhs.sy * rhs, lhs.ex * rhs, lhs.ey * rhs);
}

inline const Float4 &operator*=(Float4 &lhs, float rhs) {
  lhs.sx *= rhs;
  lhs.sy *= rhs;
  lhs.ex *= rhs;
  lhs.ey *= rhs;
  return lhs;
}
inline Float4 &operator/=(Float4 &lhs, float rhs) {
  lhs.sx /= rhs;
  lhs.sy /= rhs;
  lhs.ex /= rhs;
  lhs.ey /= rhs;
  return lhs;
}

inline Float4 operator+(const Float4 &lhs, const Float2 &rhs) {
  return Float4(lhs.sx + rhs.x, lhs.sy + rhs.y, lhs.ex + rhs.x, lhs.ey + rhs.y);
}

inline Float4 &operator-=(Float4 &lhs, const Float2 &rhs) {
  lhs.sx -= rhs.x;
  lhs.sy -= rhs.y;
  lhs.ex -= rhs.x;
  lhs.ey -= rhs.y;
  return lhs;
}

inline bool operator==(const Float4 &lhs, const Float4 &rhs) {
  return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

inline Float2 clamp(const Float2 &val, const Float4 &bounds) {
  return Float2(clamp(val.x, bounds.sx, bounds.ex), clamp(val.y, bounds.sy, bounds.ey));
}

/*************
 * Computational geometry
 */

float len(const Float2 &in);
Float2 normalize(const Float2 &in);
float dot(const Float2 &lhs, const Float2 &rhs);

float getAngle(const Float2 &in);
Float2 makeAngle(const float &in);

bool linelineintersect(const Float4 &lhs, const Float4 &rhs);
float linelineintersectpos(const Float4 &lhs, const Float4 &rhs);

float linepointdistance(const Float4 &lhs, const Float2 &rhs);

int whichSide(const Float4 &f4, const Float2 &pta);

pair<Float2, float> fitInside(const Float4 &objbounds, const Float4 &goalbounds);

Float2 getCentroid(const vector<Float2> &are);

int inPath(const Float2 &point, const vector<Float2> &path);

float getArea(const vector<Float2> &are);

bool pathReversed(const vector<Float2> &path);

Float2 reflect(const Float2 &incoming, float normal);
float reflect(float incoming, float normal);

vector<Float2> generateCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints);
float bezinterp(float x0, float x1, float x2, float x3, float t);

/*************
 * Bounding box
 */

Float4 startFBoundBox();

void addToBoundBox(Float4 *bbox, float x, float y);
void addToBoundBox(Float4 *bbox, const Float2 &point);
void addToBoundBox(Float4 *bbox, const Float4 &rect);

void expandBoundBox(Float4 *bbox, float factor);

/*************
 * Math
 */

inline int round(float in) { return int(floor(in + 0.5)); }

inline bool isInside(const Float4 &lhs, const Float2 &rhs) {
  return !(rhs.x < lhs.sx || rhs.x >= lhs.ex || rhs.y < lhs.sy || rhs.y >= lhs.ey);
}

inline Float4 boxAround(const Float2 &lhs, float radius) {
  return Float4(lhs.x - radius, lhs.y - radius, lhs.x + radius, lhs.y + radius);
}

Float2 rotate(const Float2 &in, float ang);

// square inscribed in the rect
Float4 squareInside(const Float4 &in);

inline float approach(float start, float target, float delta) {
  if(abs(start - target) <= delta)
    return target;
  else if(start < target)
    return start + delta;
  else if(start > target)
    return start - delta;
  else
    CHECK(0);  // oh god bear is driving car how can this be
}

Float4 extend(const Float4 &in, float amount);
Float4 contract(const Float4 &in, float amount);

Float2 lerp(const Float2 &lhs, const Float2 &rhs, float dist);
Float2 lerp(const Float4 &movement, float dist);

/*************
 * Matrixtastic
 */

class Float2;
class Transform2d {
public:
  float m[3][3];

  void hflip();
  void vflip();
  void dflip();
  
  float det();
  float detchunk(int x, int y);
  
  void invert();
  
  void transform(Float2 *pos) const;
  
  void display() const;

  Transform2d();
};

Transform2d operator*(const Transform2d &lhs, const Transform2d &rhs);
Transform2d &operator*=(Transform2d &lhs, const Transform2d &rhs);

Transform2d t2d_identity();
Transform2d t2d_flip(bool h, bool v, bool d);
Transform2d t2d_rotate(float rads);

#endif
