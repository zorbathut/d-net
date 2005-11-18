#ifndef DNET_COMPOSITE_IMP
#define DNET_COMPOSITE_IMP

#include <cmath>
#include <algorithm>

#include "debug.h"

using namespace std;

#define Type1 typename T::T1
#define Type2 typename T::T2
#define Type4 typename T::T4

/*************
 * Angles
 */
 
template <typename T> Type1 imp_len(const Type2 &in) {
    return sqrt(in.x * in.x + in.y * in.y);
}

template <typename T> Type2 imp_normalize(const Type2 &in) {
    return in / len(in);
}

template <typename T> Type1 imp_getAngle(const Type2 &in) {
    return T::atan2(in.y, in.x);
}
template <typename T> Type2 imp_makeAngle(const Type1 &in) {
    return Type2(T::cos(in), T::sin(in));
}

template <typename T> int imp_whichSide( const Type4 &f4, const Type2 &pta ) {
    Type1 ax = f4.ex - f4.sx;
    Type1 ay = f4.ey - f4.sy;
    Type1 bx = pta.x - f4.sx;
    Type1 by = pta.y - f4.sy;
    swap(ax, ay);
    ax *= -1;
    Type1 rv = ax * bx + ay * by;
    if( rv < 0 ) return -1;
    else if( rv > 0 ) return 1;
    else return 0;
}

/*************
 * Bounding box
 */

template <typename T> Type4 imp_startBoundBox() {
    return Type4(1000000000, 1000000000, -1000000000, -1000000000);
};

template <typename T> void imp_addToBoundBox(Type4 *bbox, Type1 x, Type1 y) {
    bbox->sx = min(bbox->sx, x);
    bbox->sy = min(bbox->sy, y);
    bbox->ex = max(bbox->ex, x);
    bbox->ey = max(bbox->ey, y);
};
template <typename T> void imp_addToBoundBox(Type4 *bbox, const Type2 &point) {
    addToBoundBox(bbox, point.x, point.y);
};
template <typename T> void imp_addToBoundBox(Type4 *bbox, const Type4 &rect) {
    CHECK(rect.isNormalized());
    addToBoundBox(bbox, rect.sx, rect.sy);
    addToBoundBox(bbox, rect.ex, rect.ey);
};

template <typename T> void imp_expandBoundBox(Type4 *bbox, Type1 factor) {
    Type1 x = bbox->ex - bbox->sx;
    Type1 y = bbox->ey - bbox->sy;
    Type1 xc = ( bbox->sx + bbox->ex ) / 2;
    Type1 yc = ( bbox->sy + bbox->ey ) / 2;
    x *= factor;
    y *= factor;
    x /= 2;
    y /= 2;
    bbox->sx = xc - x;
    bbox->sy = yc - y;
    bbox->ex = xc + x;
    bbox->ey = yc + y;
}

/*************
 * Computational geometry
 */

template <typename T> pair<Type2, float> imp_fitInside(const Type4 &objbounds, const Type4 &goalbounds) {
    Type1 xscale = (goalbounds.ex - goalbounds.sx) / (objbounds.ex - objbounds.sx);
    Type1 yscale = (goalbounds.ey - goalbounds.sy) / (objbounds.ey - objbounds.sy);
    Type1 scale = min(xscale, yscale);
    Type1 goalx = (goalbounds.sx + goalbounds.ex) / 2;
    Type1 goaly = (goalbounds.sy + goalbounds.ey) / 2;
    Type1 objx = (objbounds.sx + objbounds.ex) / 2;
    Type1 objy = (objbounds.sy + objbounds.ey) / 2;
    return make_pair(Type2(goalx - objx * scale, goaly - objy * scale), scale);
}

template <typename T> bool imp_linelineintersect( Type1 x1, Type1 y1, Type1 x2, Type1 y2, Type1 x3, Type1 y3, Type1 x4, Type1 y4 ) {
	Type1 denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
    if(denom == 0)
        return false;
	Type1 ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	Type1 ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
    return ua > 0 && ua < 1 && ub > 0 && ub < 1;
}
template <typename T> bool imp_linelineintersect( const Type4 &lhs, const Type4 &rhs ) {
	return imp_linelineintersect<T>( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}
template <typename T> Type1 imp_linelineintersectpos( Type1 x1, Type1 y1, Type1 x2, Type1 y2, Type1 x3, Type1 y3, Type1 x4, Type1 y4 ) {
	Type1 denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
	Type1 ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	Type1 ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	if( ua > 0 && ua < 1 && ub > 0 && ub < 1 )
		return ua;
	else
		return 2;
}
template <typename T> Type1 imp_linelineintersectpos( const Type4 &lhs, const Type4 &rhs ) {
	return imp_linelineintersectpos<T>( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}

/*
inline int whichSide( const Coord4 &f4, const Coord2 &pta ) {
    Coord ax = f4.ex - f4.sx;
    Coord ay = f4.ey - f4.sy;
    Coord bx = pta.x - f4.sx;
    Coord by = pta.y - f4.sy;
    swap(ax, ay);
    ax *= -1;
    Coord rv = ax * bx + ay * by;
    if( rv < 0 ) return -1;
    else if( rv > 0 ) return 1;
    else return 0;
}

inline Coord2 lerp( const Coord2 &start, const Coord2 &delta, Coord time ) {
    return Coord2( start.x + delta.x * time, start.y + delta.y * time );
}
inline Coord4 lerp( const Coord4 &start, const Coord4 &delta, Coord time ) {
    return Coord4( start.sx + delta.sx * time, start.sy + delta.sy * time, start.ex + delta.ex * time, start.ey + delta.ey * time );
}

inline Coord4 snapToEnclosingGrid(Coord4 orig, Coord grid) {
    orig.sx = ceil(orig.sx/grid - 1) * grid;
    orig.sy = ceil(orig.sy/grid - 1) * grid;
    orig.ex = ceil(orig.ex/grid) * grid;
    orig.ey = ceil(orig.ey/grid) * grid;
    return orig;
}

Coord4 startCBoundBox();

void addToBoundBox(Coord4 *bbox, Coord x, Coord y);
void addToBoundBox(Coord4 *bbox, const Coord2 &point);
void addToBoundBox(Coord4 *bbox, const Coord4 &rect);

void expandBoundBox(Coord4 *bbox, Coord factor);

Coord4 getBoundBox(const vector<Coord2> &path);

// returns -1 if the point is actually inside the path, but the path is reversed
int inPath(const Coord2 &point, const vector<Coord2> &path);
bool roughInPath(const Coord2 &point, const vector<Coord2> &path, int goal);

Coord2 getPointIn(const vector<Coord2> &path);

bool pathReversed(const vector<Coord2> &path);

enum { PR_SEPARATE, PR_INTERSECT, PR_LHSENCLOSE, PR_RHSENCLOSE };

int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs);

Coord distanceFromLine(const Coord4 &line, const Coord2 &pt);
bool colinear(const Coord4 &line, const Coord2 &pt);

Coord getArea(const vector<Coord2> &are);
*/

#endif
