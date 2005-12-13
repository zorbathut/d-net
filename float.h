#ifndef DNET_FLOAT
#define DNET_FLOAT

#include <utility>

#include "cfcommon.h"

using namespace std;

/*************
 * Float2/Float4 classes and operators
 */

class Float2 {
public:
  float x, y;
  Float2() { };
  Float2( float in_x, float in_y ) :
    x( in_x ), y( in_y ) { };
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
  Float4() { };
  Float4(const Float2 &s, const Float2 &e) :
    sx(s.x), sy(s.y), ex(e.x), ey(e.y) { };
  Float4( float in_sx, float in_sy, float in_ex, float in_ey ) :
    sx( in_sx ), sy( in_sy ), ex( in_ex ), ey( in_ey ) { };

  inline Float4 normalize() const {
    Float4 cp = *this;
    float tmp;
    if( cp.sx > cp.ex ) {
      tmp = cp.sx;
      cp.sx = cp.ex;
      cp.ex = tmp;
    }
    if( cp.sy > cp.ey ) {
      tmp = cp.sy;
      cp.sy = cp.ey;
      cp.ey = tmp;
    }
    return cp;
  }

  bool isNormalized() const {
    return sx <= ex && sy <= ey;
  }
};

inline Float4 operator+( const Float4 &lhs, const Float4 &rhs ) {
  return Float4( lhs.sx + rhs.sx, lhs.sy + rhs.sy, lhs.ex + rhs.ex, lhs.ey + rhs.ey );
}
inline Float4 operator*( const Float4 &lhs, float rhs ) {
  return Float4( lhs.sx * rhs, lhs.sy * rhs, lhs.ex * rhs, lhs.ey * rhs );
}
inline const Float4 &operator*=( Float4 &lhs, float rhs ) {
  lhs.sx *= rhs;
  lhs.sy *= rhs;
  lhs.ex *= rhs;
  lhs.ey *= rhs;
  return lhs;
}
inline Float4 &operator/=( Float4 &lhs, float rhs ) {
  lhs.sx /= rhs;
  lhs.sy /= rhs;
  lhs.ex /= rhs;
  lhs.ey /= rhs;
  return lhs;
}

inline bool operator==( const Float4 &lhs, const Float4 &rhs ) {
  return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

/*************
 * Computational geometry
 */

float len(const Float2 &in);
Float2 normalize(const Float2 &in);

float getAngle(const Float2 &in);
Float2 makeAngle(const float &in);

bool linelineintersect( const Float4 &lhs, const Float4 &rhs );
float linelineintersectpos( const Float4 &lhs, const Float4 &rhs );

int whichSide( const Float4 &f4, const Float2 &pta );

pair<Float2, float> fitInside(const Float4 &objbounds, const Float4 &goalbounds);

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

inline bool isinside(const Float4 &lhs, const Float2 &rhs) {
  return !(rhs.x < lhs.sx || rhs.x >= lhs.ex || rhs.y < lhs.sy || rhs.y >= lhs.ey);
}

inline Float4 boxaround(const Float2 &lhs, float radius) {
  return Float4(lhs.x - radius, lhs.y - radius, lhs.x + radius, lhs.y + radius);
}

bool linelineintersect( const Float4 &lhs, const Float4 &rhs );
float linelineintersectpos( const Float4 &lhs, const Float4 &rhs );

Float2 rotate(const Float2 &in, float ang);

#endif
