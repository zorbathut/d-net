
#include "float.h"

#include "composite-imp.h"

class Floats {
public:
    typedef float T1;
    typedef Float2 T2;
    typedef Float4 T4;
    
    static float atan2(float a, float b) { return ::atan2(a, b); }
    static float sin(float a) { return fsin(a); };
    static float cos(float a) { return fcos(a); };
};

/*************
 * Computational geometry
 */

float len(const Float2 &in) { return imp_len<Floats>(in); }
Float2 normalize(const Float2 &in) { return imp_normalize<Floats>(in); }

float getAngle(const Float2 &in) { return imp_getAngle<Floats>(in); };
Float2 makeAngle(const float &in) { return imp_makeAngle<Floats>(in); };

int whichSide( const Float4 &f4, const Float2 &pta ) { return imp_whichSide<Floats>(f4, pta); };

pair<Float2, float> fitInside(const Float4 &objbounds, const Float4 &goalbounds) { return imp_fitInside<Floats>(objbounds, goalbounds); }

/*************
 * Bounding box
 */

Float4 startFBoundBox() { return imp_startBoundBox<Floats>(); };

void addToBoundBox(Float4 *bbox, float x, float y) { return imp_addToBoundBox<Floats>(bbox, x, y); };
void addToBoundBox(Float4 *bbox, const Float2 &point) { return imp_addToBoundBox<Floats>(bbox, point); };
void addToBoundBox(Float4 *bbox, const Float4 &rect) { return imp_addToBoundBox<Floats>(bbox, rect); };

void expandBoundBox(Float4 *bbox, float factor) { return imp_expandBoundBox<Floats>(bbox, factor); };

/*************
 * Fast sin/cos
 */

float sin_table[ SIN_TABLE_SIZE + 1 ];

class sinTableMaker {
public:
	sinTableMaker() {
		for( int i = 0; i < SIN_TABLE_SIZE + 1; i++ ) {
			sin_table[ i ] = sin( i * PI / SIN_TABLE_SIZE / 2 );
		}

		//for( float x = 0; x < PI * 2; x += 0.01 )
			//dprintf( "%f-%f, %f-%f\n", fsin( x ), sin( x ), fcos( x ), cos( x ) );
	}
};

sinTableMaker sinInit;

/*************
 * Math
 */

bool linelineintersect( const Float4 &lhs, const Float4 &rhs ) { return imp_linelineintersect<Floats>(lhs, rhs); };
float linelineintersectpos( const Float4 &lhs, const Float4 &rhs ) { return imp_linelineintersectpos<Floats>(lhs, rhs); };
















#if 0

/*************
 * Computational geometry
 */

inline bool rectrectintersect( const Float4 &lhs, const Float4 &rhs ) {
	TEST( lhs.isNormalized() );
	TEST( rhs.isNormalized() );
	if( lhs.ex <= rhs.sx || rhs.ex <= lhs.sx || lhs.ey <= rhs.sy || rhs.ey <= lhs.sy )
		return false;
	return true;
}

inline bool linelineintersect( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 ) {
	float denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
    float ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	float ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}
inline bool linelineintersect( const Float4 &lhs, const Float4 &rhs ) {
	return linelineintersect( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}

inline int whichSide( const Float4 &f4, const Float2 &pta ) {
    float ax = f4.ex - f4.sx;
    float ay = f4.ey - f4.sy;
    float bx = pta.x - f4.sx;
    float by = pta.y - f4.sy;
    swap(ax, ay);
    ax *= -1;
    float rv = ax * bx + ay * by;
    if( rv < 0 ) return -1;
    else if( rv > 0 ) return 1;
    else return 0;
}

inline float getAngle(const Float2 &in) {
    return atan2(in.y, in.x);
}
inline Float2 makeAngle(float in) {
    return Float2(fcos(in), fsin(in));
}

inline bool isinside(const Float4 &lhs, const Float2 &rhs) {
    return !(rhs.x < lhs.sx || rhs.x >= lhs.ex || rhs.y < lhs.sy || rhs.y >= lhs.ey);
}

inline Float4 boxaround(const Float2 &lhs, float radius) {
    return Float4(lhs.x - radius, lhs.y - radius, lhs.x + radius, lhs.y + radius);
}

// Returns ( (xtrans, ytrans), scale)
pair<Float2, float> fitInside(const Float4 &objbounds, const Float4 &goalbounds);

/*************
 * Math
 */

inline Float4 lerp( const Float4 &start, const Float4 &delta, float time ) {
    return Float4( start.sx + delta.sx * time, start.sy + delta.sy * time, start.ex + delta.ex * time, start.ey + delta.ey * time );
}

#endif
