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
    long long raw() const { return d; };
    
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

    Float2 toFloat() const {
        return Float2(x.toFloat(), y.toFloat());
    }

    Coord2() { };
    Coord2(const Coord &ix, const Coord &iy) : x(ix), y(iy) { };
    Coord2(const Coord2 &rhs) : x(rhs.x), y(rhs.y) { };
    explicit Coord2(const Float2 &rhs) : x(rhs.x), y(rhs.y) { };
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

inline Coord len(const Coord2 &in) {
    return sqrt(in.x * in.x + in.y * in.y);
}

inline Coord2 normalize(const Coord2 &in) {
    return in / len(in);
}

inline Coord getAngle(const Coord2 &in) {
    return Coord(atan2(in.y.toFloat(), in.x.toFloat()));
}
inline Coord2 makeAngle(const Coord &in) {
    return Coord2(cfcos(in), cfsin(in));
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

inline bool verboselinelineintersect( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float *denom, float *ua, float *ub ) {
	*denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
    dprintf("%f\n", *denom);
    *ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / *denom;
    dprintf("%f\n", *ua);
	*ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / *denom;
    dprintf("%f\n", *ub);
	return *ua >= 0 && *ua <= 1 && *ub >= 0 && *ub <= 1;
}
inline bool linelineintersect( Coord x1, Coord y1, Coord x2, Coord y2, Coord x3, Coord y3, Coord x4, Coord y4 ) {
    //dprintf("%f,%f %f,%f vs %f,%f %f,%f\n",
        //x1.toFloat(), y1.toFloat(), x2.toFloat(), y2.toFloat(), 
        //x3.toFloat(), y3.toFloat(), x4.toFloat(), y4.toFloat());
	Coord denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
    if(denom == 0)
        return false;
    //dprintf("%f\n", denom.toFloat());
	Coord ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
    //dprintf("%f\n", ua.toFloat());
	Coord ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
    //dprintf("%f\n", ub.toFloat());
    bool rv = (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1);
    if(rv != linelineintersect(x1.toFloat(), y1.toFloat(), x2.toFloat(), y2.toFloat(), 
        x3.toFloat(), y3.toFloat(), x4.toFloat(), y4.toFloat())) {
            dprintf("%f,%f %f,%f vs %f,%f %f,%f\n",
                x1.toFloat(), y1.toFloat(), x2.toFloat(), y2.toFloat(), 
                x3.toFloat(), y3.toFloat(), x4.toFloat(), y4.toFloat());
            dprintf("%f\n", denom.toFloat());
            dprintf("%f\n", ua.toFloat());
            dprintf("%f\n", ub.toFloat());
            float fdenom, fua, fub;
            verboselinelineintersect(x1.toFloat(), y1.toFloat(), x2.toFloat(), y2.toFloat(), 
                x3.toFloat(), y3.toFloat(), x4.toFloat(), y4.toFloat(), &fdenom, &fua, &fub);
            dprintf("---");
            dprintf("%f\n", (x2-x1).toFloat());
            dprintf("%f\n", (y1-x3).toFloat());
            dprintf("%f\n", (y2-y1).toFloat());
            dprintf("%f\n", (x1-x3).toFloat());
            dprintf("%f\n", (( x2 - x1 ) * ( y1 - y3 )).toFloat());
            dprintf("%f\n", (( y2 - y1 ) * ( x1 - x3 )).toFloat());
            dprintf("%f\n", ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ).toFloat());
            dprintf("%f\n", ( ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom ).toFloat());
            float iua = abs(fua - ua.toFloat());
            float iub = abs(fub - ub.toFloat());
            float iud = abs(fdenom - denom.toFloat()) / min(abs(fdenom), abs(denom.toFloat()));
            dprintf("%e\n", abs(fdenom - denom.toFloat()));
            dprintf("%e %e %e\n", iua, iub, iud);
            if(iua < 1e-6 && iub < 1e-6)
                return rv;
            CHECK(0);
    }
    return rv;
}
inline bool linelineintersect( const Coord4 &lhs, const Coord4 &rhs ) {
	return linelineintersect( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}
Coord linelineintersectpos( Coord x1, Coord y1, Coord x2, Coord y2, Coord x3, Coord y3, Coord x4, Coord y4 );
inline Coord linelineintersectpos( const Coord4 &lhs, const Coord4 &rhs ) {
	return linelineintersectpos( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}

inline Coord2 lerp( const Coord2 &start, const Coord2 &delta, Coord time ) {
    return Coord2( start.x + delta.x * time, start.y + delta.y * time );
}
inline Coord4 lerp( const Coord4 &start, const Coord4 &delta, Coord time ) {
    return Coord4( start.sx + delta.sx * time, start.sy + delta.sy * time, start.ex + delta.ex * time, start.ey + delta.ey * time );
}

Coord4 startCBoundBox();

void addToBoundBox(Coord4 *bbox, Coord x, Coord y);
void addToBoundBox(Coord4 *bbox, const Coord2 &point);
void addToBoundBox(Coord4 *bbox, const Coord4 &rect);

// returns -1 if the point is actually inside the path, but the path is reversed
int inPath(const Coord2 &point, const vector<Coord2> &path);

Coord2 getPointIn(const vector<Coord2> &path);

enum { PR_SEPARATE, PR_INTERSECT, PR_LHSENCLOSE, PR_RHSENCLOSE };

int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

#endif

