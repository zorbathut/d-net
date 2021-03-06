#ifndef DNET_COORD
#define DNET_COORD

#include "cfcommon.h"
#include "util.h"

using namespace std;

class Float2;
class Float4;

// 64-bit int in 32:32 fixed-point format
class Coord {
  friend inline Coord coordExplicit(long long lhs);
  friend inline void operator+=(Coord &lhs, const Coord &rhs);
  friend inline void operator-=(Coord &lhs, const Coord &rhs);
  friend inline void operator*=(Coord &lhs, const Coord &rhs);
  friend inline void operator/=(Coord &lhs, const Coord &rhs);
  friend inline Coord operator-(const Coord &lhs);
  friend inline Coord operator+(const Coord &lhs, const Coord &rhs);
  friend inline Coord operator-(const Coord &lhs, const Coord &rhs);
  friend inline Coord operator*(const Coord &lhs, const Coord &rhs);
  friend inline Coord operator/(const Coord &lhs, const Coord &rhs);
  friend inline Coord operator<<(const Coord &lhs, int rhs);
  friend inline Coord operator>>(const Coord &lhs, int rhs);
  friend inline bool operator==(const Coord &lhs, const Coord &rhs);
  friend inline bool operator!=(const Coord &lhs, const Coord &rhs);
  friend inline bool operator<(const Coord &lhs, const Coord &rhs);
  friend inline bool operator<=(const Coord &lhs, const Coord &rhs);
  friend inline bool operator>(const Coord &lhs, const Coord &rhs);
  friend inline bool operator>=(const Coord &lhs, const Coord &rhs);
  friend inline Coord sqrt(const Coord &in);
  friend inline Coord floor(const Coord &in);
  friend Coord mod(const Coord &a, const Coord &b);
  friend inline Coord pow(const Coord &lhs, const Coord &rhs);
  friend inline Coord exp(const Coord &lhs);
  friend inline Coord log(const Coord &lhs);
  
private:
  long long d;

public:
  Coord() { };
  Coord(const Coord &rhs) : d(rhs.d) { };
  Coord(int rhs) { d = (long long)rhs * (1LL << 32); }
  Coord(unsigned int rhs) { d = (long long)rhs * (1LL << 32); }
  Coord(long rhs) { d = (long long)rhs * (1LL << 32); }
  Coord(unsigned long rhs) { d = (long long)rhs * (1LL << 32); }
  Coord(long long rhs) { CHECK(abs(rhs) < (1LL << 31)); d = rhs * (1LL << 32); }
  Coord(float rhs) { d = (long long)(rhs * (1LL << 32)); }
  Coord(double rhs) { d = (long long)(rhs * (1LL << 32)); }
  
  float toFloat() const { return (float)d / (1LL << 32); }
  int toInt() const { CHECK(Coord(int(d >> 32)).d == d); return d >> 32; }
  long long raw() const { return d; };
  string rawstr() const;
  
  ~Coord() { }; // lol no.
};

inline Coord coordExplicit(long long lhs) {
  Coord coord;
  coord.d = lhs;
  return coord;
}
Coord coordFromRawstr(const string &lhs);

inline void operator+=(Coord &lhs, const Coord &rhs) { lhs.d += rhs.d; }
inline void operator-=(Coord &lhs, const Coord &rhs) { lhs.d -= rhs.d; }
inline void operator*=(Coord &lhs, const Coord &rhs) { lhs = lhs * rhs; }
inline void operator/=(Coord &lhs, const Coord &rhs) { lhs = lhs / rhs; }

inline Coord operator-(const Coord &lhs) {
  return coordExplicit(-lhs.d);
}

inline Coord operator+(const Coord &lhs, const Coord &rhs) {
  return coordExplicit(lhs.d + rhs.d);
}

inline Coord operator-(const Coord &lhs, const Coord &rhs) {
  return coordExplicit(lhs.d - rhs.d);
}

inline Coord operator<<(const Coord &lhs, int rhs) {
  return coordExplicit(lhs.d << rhs);
}

inline Coord operator>>(const Coord &lhs, int rhs) {
  return coordExplicit(lhs.d >> rhs);
}

inline Coord floor(const Coord &in) {
  return in.d >> 32;
}

inline Coord ceil(const Coord &in) {
  return -floor(-in);
}

// TODO: improve?
inline Coord operator*(const Coord &lhs, const Coord &rhs) {
  /*bool neg = false;
  long long ld = lhs.d;
  long long rd = rhs.d;
  if(ld < 0) { neg = !neg; ld = -ld; }
  if(rd < 0) { neg = !neg; rd = -rd; }
  unsigned int lh = (unsigned int)(ld >> 32);
  unsigned int ll = (unsigned int)ld;
  unsigned int rh = (unsigned int)(rd >> 32);
  unsigned int rl = (unsigned int)rd;
  unsigned long long rv = 0;
  rv += ((unsigned long long)ll * (unsigned long long)rl) >> 32;
  rv += ((unsigned long long)ll * (unsigned long long)rh);
  rv += ((unsigned long long)lh * (unsigned long long)rl);
  rv += ((unsigned long long)lh * (unsigned long long)rh) << 32;
  if(neg) rv = -rv;
  return coordExplicit(rv);*/
  return coordExplicit((long long)(((long double)lhs.d * (long double)rhs.d) / (1LL << 32)));
}

// TODO: improve?
inline Coord operator/(const Coord &lhs, const Coord &rhs) {
  //dprintf("op/ in!\n");
  Coord rv = coordExplicit((long long)(((long double)lhs.d / (long double)rhs.d) * (1LL << 32)));
  //dprintf("  %f %f %f %d\n", lhs.toFloat(), rhs.toFloat(), rv.toFloat(), 1234);
  //dprintf("  %lld %lld %lld %d\n", lhs.d, rhs.d, rv.d, 1234);
  //dprintf("op/ out!\n");
  return rv;
}

inline Coord pow(const Coord &lhs, const Coord &rhs) {
  return coordExplicit((long long)(pow((long double)lhs.d / (1LL << 32), (long double)rhs.d / (1LL << 32)) * (1LL << 32)));
}

inline Coord exp(const Coord &lhs) {
  return exp(lhs.toFloat());
}

inline Coord log(const Coord &lhs) {
  return log(lhs.toFloat());
}

inline bool operator==(const Coord &lhs, const Coord &rhs) {
  return lhs.d == rhs.d;
}
inline bool operator!=(const Coord &lhs, const Coord &rhs) {
  return lhs.d != rhs.d;
}
inline bool operator<(const Coord &lhs, const Coord &rhs) {
  return lhs.d < rhs.d;
}
inline bool operator<=(const Coord &lhs, const Coord &rhs) {
  return lhs.d <= rhs.d;
}
inline bool operator>(const Coord &lhs, const Coord &rhs) {
  return lhs.d > rhs.d;
}
inline bool operator>=(const Coord &lhs, const Coord &rhs) {
  return lhs.d >= rhs.d;
}

inline long long isqrt64(long long in) {
  return (long long)sqrt((long double)in);
}

inline Coord sqrt(const Coord &in) {
  return coordExplicit(isqrt64(in.d) << 16);
}

inline Coord cfsin(float in) {
  return Coord(fsin(in));
}
inline Coord cfcos(float in) {
  return Coord(fcos(in));
}

inline Coord cfsin(Coord in) {
  return cfsin(in.toFloat());
}
inline Coord cfcos(Coord in) {
  return cfcos(in.toFloat());
}

inline Coord abs(const Coord &in) {
  if(in < 0)
    return -in;
  return in;
}

inline Coord round(const Coord &in) {
  if(in.raw() >= 0)
    return floor(in + Coord(1) / 2);
  else
    return -round(-in);
}

class Coord2 {
public:
  Coord x, y;

  Float2 toFloat() const;

  Coord2() { };
  Coord2(const Coord &ix, const Coord &iy) : x(ix), y(iy) { };
  Coord2(float ix, float iy) : x(ix), y(iy) { };
  Coord2(const Coord2 &rhs) : x(rhs.x), y(rhs.y) { };
  explicit Coord2(const Float2 &rhs);
  
  string rawstr() const;
};

inline Coord2 operator+(const Coord2 &lhs, const Coord2 &rhs) {
  return Coord2(lhs.x + rhs.x, lhs.y + rhs.y);
}
inline Coord2 operator-(const Coord2 &lhs, const Coord2 &rhs) {
  return Coord2(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline Coord2 operator*(const Coord2 &lhs, const Coord &rhs) {
  return Coord2(lhs.x * rhs, lhs.y * rhs);
}
inline Coord2 operator/(const Coord2 &lhs, const Coord &rhs) {
  return Coord2(lhs.x / rhs, lhs.y / rhs);
}

inline Coord2 &operator+=(Coord2 &lhs, const Coord2 &rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}
inline Coord2 &operator-=(Coord2 &lhs, const Coord2 &rhs) {
  lhs.x -= rhs.x;
  lhs.y -= rhs.y;
  return lhs;
}
inline Coord2 &operator*=(Coord2 &lhs, const Coord &rhs) {
  lhs.x *= rhs;
  lhs.y *= rhs;
  return lhs;
}
inline Coord2 &operator/=(Coord2 &lhs, const Coord &rhs) {
  lhs.x /= rhs;
  lhs.y /= rhs;
  return lhs;
}

inline Coord2 operator-(const Coord2 &lhs) {
  return Coord2(-lhs.x, -lhs.y);
}

inline bool operator==(const Coord2 &lhs, const Coord2 &rhs) {
  if(lhs.x != rhs.x) return false;
  if(lhs.y != rhs.y) return false;
  return true;
}
inline bool operator!=(const Coord2 &lhs, const Coord2 &rhs) {
  return !(lhs == rhs);
}
inline bool operator<(const Coord2 &lhs, const Coord2 &rhs) {
  if(lhs.x != rhs.x) return lhs.x < rhs.x;
  if(lhs.y != rhs.y) return lhs.y < rhs.y;
  return false;
}
inline bool operator<=(const Coord2 &lhs, const Coord2 &rhs) {
  return (lhs < rhs) || (lhs == rhs);
}
inline bool operator>(const Coord2 &lhs, const Coord2 &rhs) {
  return rhs < lhs;
}
inline bool operator>=(const Coord2 &lhs, const Coord2 &rhs) {
  return rhs <= lhs;
}

class Coord4 {
public:
  Coord sx, sy, ex, ey;

  inline bool isNormalized() const {
    return sx <= ex && sy <= ey;
  }
  
  Coord2 s() const {
    return Coord2(sx, sy);
  }
  Coord2 e() const {
    return Coord2(ex, ey);
  }
  
  Coord span_x() const {
    return ex - sx;
  }
  Coord span_y() const {
    return ey - sy;
  }
  
  Coord2 midpoint() const {
    return (s() + e()) / 2;
  }
  
  Coord2 vector() const {
    return e() - s();
  }
  
  Float4 toFloat() const;

  Coord4() { };
  Coord4(const Coord &isx, const Coord &isy, const Coord &iex, const Coord &iey) : sx(isx), sy(isy), ex(iex), ey(iey) { };
  Coord4(const Coord2 &s, const Coord2 &e) : sx(s.x), sy(s.y), ex(e.x), ey(e.y) { };
  Coord4(const Coord4 &rhs) : sx(rhs.sx), sy(rhs.sy), ex(rhs.ex), ey(rhs.ey) { };
  Coord4(float isx, float isy, float iex, float iey) : sx(isx), sy(isy), ex(iex), ey(iey) { };
  explicit Coord4(const Float4 &rhs);
  
  string rawstr() const;
};

inline Coord4 operator+(const Coord4 &lhs, const Coord4 &rhs) {
  return Coord4(lhs.sx + rhs.sx, lhs.sy + rhs.sy, lhs.ex + rhs.ex, lhs.ey + rhs.ey);
}
inline Coord4 operator-(const Coord4 &lhs, const Coord4 &rhs) {
  return Coord4(lhs.sx - rhs.sx, lhs.sy - rhs.sy, lhs.ex - rhs.ex, lhs.ey - rhs.ey);
}
inline Coord4 operator*(const Coord4 &lhs, const Coord &rhs) {
  return Coord4(lhs.sx * rhs, lhs.sy * rhs, lhs.ex * rhs, lhs.ey * rhs);
}
inline Coord4 operator/(const Coord4 &lhs, const Coord &rhs) {
  return Coord4(lhs.sx / rhs, lhs.sy / rhs, lhs.ex / rhs, lhs.ey / rhs);
}

inline Coord4 &operator+=(Coord4 &lhs, const Coord4 &rhs) {
  lhs.sx += rhs.sx;
  lhs.sy += rhs.sy;
  lhs.ex += rhs.ex;
  lhs.ey += rhs.ey;
  return lhs;
}
inline Coord4 &operator-=(Coord4 &lhs, const Coord4 &rhs) {
  lhs.sx -= rhs.sx;
  lhs.sy -= rhs.sy;
  lhs.ex -= rhs.ex;
  lhs.ey -= rhs.ey;
  return lhs;
}
inline Coord4 &operator*=(Coord4 &lhs, const Coord &rhs) {
  lhs.sx *= rhs;
  lhs.sy *= rhs;
  lhs.ex *= rhs;
  lhs.ey *= rhs;
  return lhs;
}
inline Coord4 &operator/=(Coord4 &lhs, const Coord &rhs) {
  lhs.sx /= rhs;
  lhs.sy /= rhs;
  lhs.ex /= rhs;
  lhs.ey /= rhs;
  return lhs;
}

inline Coord max(Coord lhs, int rhs) {
  return max(lhs, Coord(rhs));
}
inline Coord max(int lhs, Coord rhs) {
  return max(Coord(lhs), rhs);
}

inline bool operator==(const Coord4 &lhs, const Coord4 &rhs) {
  return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

inline Coord4 operator+(const Coord4 &lhs, const Coord2 &rhs) {
  return Coord4(lhs.sx + rhs.x, lhs.sy + rhs.y, lhs.ex + rhs.x, lhs.ey + rhs.y);
}

inline Coord4 &operator-=(Coord4 &lhs, const Coord2 &rhs) {
  lhs.sx -= rhs.x;
  lhs.sy -= rhs.y;
  lhs.ex -= rhs.x;
  lhs.ey -= rhs.y;
  return lhs;
}

/*************
 * PosInfo
 */

class CPosInfo {
public:
  Coord2 pos;
  Coord d;

  CPosInfo() { };
  CPosInfo(const CPosInfo &cpi) : pos(cpi.pos), d(cpi.d) { };
  CPosInfo(const Coord2 &pos, Coord d) : pos(pos), d(d) { };
};

/*************
 * Computational geometry
 */

Coord len(const Coord2 &in);
Coord2 normalize(const Coord2 &in);
Coord dot(const Coord2 &lhs, const Coord2 &rhs);

Coord getAngle(const Coord2 &in);
Coord2 makeAngle(const Coord &in);

bool linelineintersect(const Coord4 &lhs, const Coord4 &rhs);
Coord linelineintersectpos(const Coord4 &lhs, const Coord4 &rhs);

int whichSide(const Coord4 &f4, const Coord2 &pta);

pair<Coord2, float> fitInside(const Coord4 &objbounds, const Coord4 &goalbounds);

Coord distanceFromLine(const Coord4 &line, const Coord2 &pt);

int inPath(const Coord2 &point, const vector<Coord2> &path);
bool roughInPath(const Coord2 &point, const vector<Coord2> &path, int goal);

Coord2 getPointIn(const vector<Coord2> &path);

enum { PR_SEPARATE, PR_INTERSECT, PR_LHSENCLOSE, PR_RHSENCLOSE };
int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

extern bool getDifferenceInstaCrashy;
vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

Coord getArea(const vector<Coord2> &are);
Coord2 getCentroid(const vector<Coord2> &are);
Coord getPerimeter(const vector<Coord2> &are);
Coord4 getBoundBox(const vector<Coord2> &are);

bool pathReversed(const vector<Coord2> &path);

bool colinear(const Coord4 &line, const Coord2 &pt);

Coord2 reflect(const Coord2 &incoming, Coord normal);
Coord reflect(Coord incoming, Coord normal);

Coord ang_dist(Coord lhs, Coord rhs);
Coord ang_approach(const Coord &cur, const Coord &goal, const Coord &amount);

/*************
 * Bounding box
 */
 
Coord4 startCBoundBox();

inline void addToBoundBox(Coord4 *bbox, Coord x, Coord y) {
  bbox->sx = min(bbox->sx, x);
  bbox->sy = min(bbox->sy, y);
  bbox->ex = max(bbox->ex, x);
  bbox->ey = max(bbox->ey, y);
}
inline void addToBoundBox(Coord4 *bbox, const Coord2 &point) {
  addToBoundBox(bbox, point.x, point.y);
}
void addToBoundBox(Coord4 *bbox, const Coord4 &rect);
void addToBoundBox(Coord4 *bbox, const vector<Coord2> &line);

void expandBoundBox(Coord4 *bbox, Coord factor);

inline Coord2 clamp(const Coord2 &val, const Coord4 &bounds) {
  return Coord2(clamp(val.x, bounds.sx, bounds.ex), clamp(val.y, bounds.sy, bounds.ey));
}

/*************
 * Math
 */

inline bool isInside(const Coord4 &lhs, const Coord2 &rhs) {
  return !(rhs.x < lhs.sx || rhs.x >= lhs.ex || rhs.y < lhs.sy || rhs.y >= lhs.ey);
}

inline Coord4 boxAround(const Coord2 &lhs, Coord radius) {
  return Coord4(lhs.x - radius, lhs.y - radius, lhs.x + radius, lhs.y + radius);
}

Coord2 rotate(const Coord2 &in, Coord ang);

Coord approach(Coord start, Coord target, Coord delta, Coord drag);
Coord2 approach(Coord2 start, Coord2 target, Coord delta);
Coord2 approach(Coord2 start, Coord2 target, Coord delta, Coord drag);

inline Coord4 snapToEnclosingGrid(Coord4 orig, Coord grid) {
  orig.sx = floor(orig.sx/grid) * grid;
  orig.sy = floor(orig.sy/grid) * grid;
  orig.ex = ceil(orig.ex/grid) * grid;
  orig.ey = ceil(orig.ey/grid) * grid;
  if(orig.sx == orig.ex)
    orig.ex += grid;
  if(orig.sy == orig.ey)
    orig.ey += grid;
  return orig;
}

inline bool boxBoxIntersect(const Coord4 &lhs, const Coord4 &rhs) {
  return !(lhs.sx > rhs.ex || lhs.ex < rhs.sx || lhs.sy > rhs.ey || lhs.ey < rhs.sy);
}

inline bool boxLineIntersect(const Coord4 &box, const Coord4 &line) {
  Coord4 lts = line;
  if(lts.sx > lts.ex)
    swap(lts.sx, lts.ex);
  if(lts.sy > lts.ey)
    swap(lts.sy, lts.ey);
  return boxBoxIntersect(box, lts);
}

Coord mod(const Coord &a, const Coord &b);

Coord2 lerp(const Coord2 &lhs, const Coord2 &rhs, Coord dist);
Coord2 lerp(const Coord4 &movement, Coord dist);
Coord4 lerp(const Coord4 &lhs, const Coord4 &rhs, Coord dist);

const Coord COORDPI = Coord(PI);

/*************
 * Adler
 */

class Adler32;
void adler(Adler32 *adl, const Coord &val);
void adler(Adler32 *adl, const Coord2 &val);
void adler(Adler32 *adl, const Coord4 &val);
void adler(Adler32 *adl, const CPosInfo &val);

#endif
