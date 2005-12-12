#ifndef DNET_COORD
#define DNET_COORD

#include <string>
#include <vector> // remove when cpath.h and fpath.h are created

#include "cfcommon.h"#include "debug.h"

using namespace std;

class Float2;
class Float4;

// 64-bit int in 32:32 fixed-point format
class Coord {
    friend inline Coord coordExplicit(long long lhs);
    friend inline Coord &operator+=(Coord &lhs, const Coord &rhs);
    friend inline Coord &operator-=(Coord &lhs, const Coord &rhs);
    friend inline Coord &operator*=(Coord &lhs, const Coord &rhs);
    friend inline Coord &operator/=(Coord &lhs, const Coord &rhs);
    friend inline Coord operator-(const Coord &lhs);
    friend inline Coord operator+(const Coord &lhs, const Coord &rhs);
    friend inline Coord operator-(const Coord &lhs, const Coord &rhs);
    friend inline Coord operator*(const Coord &lhs, const Coord &rhs);
    friend inline Coord operator/(const Coord &lhs, const Coord &rhs);
    friend inline bool operator==(const Coord &lhs, const Coord &rhs);
    friend inline bool operator!=(const Coord &lhs, const Coord &rhs);
    friend inline bool operator<(const Coord &lhs, const Coord &rhs);
    friend inline bool operator<=(const Coord &lhs, const Coord &rhs);
    friend inline bool operator>(const Coord &lhs, const Coord &rhs);
    friend inline bool operator>=(const Coord &lhs, const Coord &rhs);
    friend inline Coord sqrt(const Coord &in);
    friend inline Coord floor(const Coord &in);
    
private:
    long long d;

public:
    Coord() { };
    Coord(const Coord &rhs) : d(rhs.d) { };
    Coord(int rhs) { d = (long long)rhs << 32; }
    explicit Coord(float rhs) { d = (long long)(rhs * (1LL << 32)); }
    explicit Coord(double rhs) { d = (long long)(rhs * (1LL << 32)); }
    
    float toFloat() const { return (float)d / ( 1LL << 32 ); }
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
Coord coordExplicit(const string &lhs);

inline Coord &operator+=(Coord &lhs, const Coord &rhs) {
    lhs.d += rhs.d;
    return lhs;
}

inline Coord &operator-=(Coord &lhs, const Coord &rhs) {
    lhs.d -= rhs.d;
    return lhs;
}

inline Coord &operator*=(Coord &lhs, const Coord &rhs) {
    lhs = lhs * rhs;
    return lhs;
}

inline Coord &operator/=(Coord &lhs, const Coord &rhs) {
    lhs = lhs / rhs;
    return lhs;
}

inline Coord operator-(const Coord &lhs) {
    return coordExplicit(-lhs.d);
}

inline Coord operator+(const Coord &lhs, const Coord &rhs) {
    return coordExplicit(lhs.d + rhs.d);
}

inline Coord operator-(const Coord &lhs, const Coord &rhs) {
    return coordExplicit(lhs.d - rhs.d);
}

inline Coord floor(const Coord &in) {
    return in.d >> 32;
}

inline Coord ceil(const Coord &in) {
    return -floor(-in);
}

// TODO: improve?
inline Coord operator*(const Coord &lhs, const Coord &rhs) {
    bool neg = false;
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
    return coordExplicit(rv);
}


// TODO: improve?
inline Coord operator/(const Coord &lhs, const Coord &rhs) {
    //dprintf("op/ in!\n");
    Coord rv = coordExplicit(
    (long long)(
            ((long double)lhs.d / (long double)rhs.d) * (1LL << 32)
        )
    );
    //dprintf("  %f %f %f %d\n", lhs.toFloat(), rhs.toFloat(), rv.toFloat(), 1234);
    //dprintf("  %lld %lld %lld %d\n", lhs.d, rhs.d, rv.d, 1234);
    //dprintf("op/ out!\n");
    return rv;
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

class Coord2 {
public:
    Coord x, y;

    Float2 toFloat() const;

    Coord2() { };
    Coord2(const Coord &ix, const Coord &iy) : x(ix), y(iy) { };
    Coord2(float ix, float iy) : x(ix), y(iy) { };
    Coord2(const Coord2 &rhs) : x(rhs.x), y(rhs.y) { };
    explicit Coord2(const Float2 &rhs);
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
    
    Float4 toFloat() const;

    Coord4() { };
    Coord4(const Coord &isx, const Coord &isy, const Coord &iex, const Coord &iey) : sx(isx), sy(isy), ex(iex), ey(iey) { };
    Coord4(const Coord2 &s, const Coord2 &e) : sx(s.x), sy(s.y), ex(e.x), ey(e.y) { };
    Coord4(const Coord4 &rhs) : sx(rhs.sx), sy(rhs.sy), ex(rhs.ex), ey(rhs.ey) { };
    Coord4(float isx, float isy, float iex, float iey) : sx(isx), sy(isy), ex(iex), ey(iey) { };
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

inline Coord max(Coord lhs, int rhs) {
    return max(lhs, Coord(rhs));
}
inline Coord max(int lhs, Coord rhs) {
    return max(lhs, Coord(rhs));
}

inline bool operator==(const Coord4 &lhs, const Coord4 &rhs) {
    return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

/*************
 * Computational geometry
 */

Coord len(const Coord2 &in);
Coord2 normalize(const Coord2 &in);

Coord getAngle(const Coord2 &in);
Coord2 makeAngle(const Coord &in);

bool linelineintersect( const Coord4 &lhs, const Coord4 &rhs );
Coord linelineintersectpos( const Coord4 &lhs, const Coord4 &rhs );

int whichSide( const Coord4 &f4, const Coord2 &pta );

pair<Coord2, float> fitInside(const Coord4 &objbounds, const Coord4 &goalbounds);

Coord distanceFromLine(const Coord4 &line, const Coord2 &pt);

int inPath(const Coord2 &point, const vector<Coord2> &path);
bool roughInPath(const Coord2 &point, const vector<Coord2> &path, int goal);

Coord2 getPointIn(const vector<Coord2> &path);

bool pathReversed(const vector<Coord2> &path);

enum { PR_SEPARATE, PR_INTERSECT, PR_LHSENCLOSE, PR_RHSENCLOSE };
int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

Coord getArea(const vector<Coord2> &are);
Coord2 getCentroid(const vector<Coord2> &are);
Coord getPerimeter(const vector<Coord2> &are);
Coord4 getBoundBox(const vector<Coord2> &are);

bool colinear(const Coord4 &line, const Coord2 &pt);

/*************
 * Bounding box
 */
 
Coord4 startCBoundBox();

void addToBoundBox(Coord4 *bbox, Coord x, Coord y);
void addToBoundBox(Coord4 *bbox, const Coord2 &point);
void addToBoundBox(Coord4 *bbox, const Coord4 &rect);

void expandBoundBox(Coord4 *bbox, Coord factor);

/*************
 * Math
 */

inline Coord4 snapToEnclosingGrid(Coord4 orig, Coord grid) {
    orig.sx = ceil(orig.sx/grid - 1) * grid;
    orig.sy = ceil(orig.sy/grid - 1) * grid;
    orig.ex = ceil(orig.ex/grid) * grid;
    orig.ey = ceil(orig.ey/grid) * grid;
    return orig;
}

bool linelineintersect( const Coord4 &lhs, const Coord4 &rhs );
Coord linelineintersectpos( const Coord4 &lhs, const Coord4 &rhs );

#endif

