#ifndef DNET_COORD
#define DNET_COORD

#include "util.h"

#include <numeric>

using namespace std;

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
    
private:
    long long d;

public:
    Coord() { };
    Coord(const Coord &rhs) : d(rhs.d) { };
    Coord(int rhs) { d = (long long)rhs << 32; }
    explicit Coord(float rhs) { d = (long long)(rhs * (1LL << 32)); }
    
    float toFloat() const { return (float)d / ( 1LL << 32 ); }
    
    ~Coord() { }; // lol no.
};

inline Coord coordExplicit(long long lhs) {
    Coord coord;
    coord.d = lhs;
    return coord;
}

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

// TODO: improve?
inline Coord operator*(const Coord &lhs, const Coord &rhs) {
    return coordExplicit((lhs.d >> 16) * (rhs.d >> 16));
}

// TODO: improve?
inline Coord operator/(const Coord &lhs, const Coord &rhs) {
    return coordExplicit((lhs.d << 8) / (rhs.d >> 8));
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

inline Coord abs(const Coord &in) {
    if(in < 0)
        return -in;
    return in;
}

class Coord2 {
public:
    Coord x, y;

    Float2 toFloat() const {
        return Float2(x.toFloat(), y.toFloat());
    }

    Coord2() { };
    Coord2(const Coord &ix, const Coord &iy) : x(ix), y(iy) { };
    Coord2(const Coord2 &rhs) : x(rhs.x), y(rhs.y) { };
    explicit Coord2(const Float2 &rhs) : x(rhs.x), y(rhs.y) { };
};

inline Coord2 &operator-=(Coord2 &lhs, const Coord2 &rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

class Coord4 {
public:
    Coord sx, sy, ex, ey;

    inline bool isNormalized() const {
        return sx <= ex && sy <= ey;
    }
    
    Float4 toFloat() const {
        return Float4(sx.toFloat(), sy.toFloat(), ex.toFloat(), ey.toFloat());
    }

    Coord4() { };
    Coord4(const Coord &isx, const Coord &isy, const Coord &iex, const Coord &iey) : sx(isx), sy(isy), ex(iex), ey(iey) { };
    Coord4(const Coord2 &s, const Coord2 &e) : sx(s.x), sy(s.y), ex(e.x), ey(e.y) { };
    Coord4(const Coord4 &rhs) : sx(rhs.sx), sy(rhs.sy), ex(rhs.ex), ey(rhs.ey) { };
};

inline Coord max(Coord lhs, int rhs) {
    return max(lhs, Coord(rhs));
}
inline Coord max(int lhs, Coord rhs) {
    return max(lhs, Coord(rhs));
}

inline bool operator==(const Coord4 &lhs, const Coord4 &rhs) {
    return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

bool linelineintersect( Coord x1, Coord y1, Coord x2, Coord y2, Coord x3, Coord y3, Coord x4, Coord y4 );
inline bool linelineintersect( const Coord4 &lhs, const Coord4 &rhs ) {
	return linelineintersect( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}
Coord linelineintersectpos( Coord x1, Coord y1, Coord x2, Coord y2, Coord x3, Coord y3, Coord x4, Coord y4 );
inline Coord linelineintersectpos( const Coord4 &lhs, const Coord4 &rhs ) {
	return linelineintersectpos( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}

inline Coord4 lerp( const Coord4 &start, const Coord4 &delta, Coord time ) {
    return Coord4( start.sx + delta.sx * time, start.sy + delta.sy * time, start.ex + delta.ex * time, start.ey + delta.ey * time );
}

Coord4 startCBoundBox();

void addToBoundBox(Coord4 *bbox, Coord x, Coord y);
void addToBoundBox(Coord4 *bbox, const Coord2 &point);
void addToBoundBox(Coord4 *bbox, const Coord4 &rect);

#endif
