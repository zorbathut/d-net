#ifndef DNET_COMPOSITE_IMP
#define DNET_COMPOSITE_IMP

#include "debug.h"

#include <algorithm>
#include <cmath>

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


template <typename T> Type1 imp_getArea(const vector<Type2> &are) {
  Type1 totare = 0;
  for(int i = 0; i < are.size(); i++) {
    int j = (i + 1) % are.size();
    totare += are[i].x * are[j].y - are[j].x * are[i].y;
  }
  totare /= 2;
  return totare;
}
template <typename T> Type2 imp_getCentroid(const vector<Type2> &are) {
  Type2 centroid(0, 0);
  for(int i = 0; i < are.size(); i++) {
    int j = (i + 1) % are.size();
    Type1 common = (are[i].x * are[j].y - are[j].x * are[i].y);
    centroid += (are[i] + are[j]) * common;
  }
  centroid /= 6 * imp_getArea<T>(are);
  return centroid;
}

#endif
