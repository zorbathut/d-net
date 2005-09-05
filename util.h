#ifndef DNET_UTIL
#define DNET_UTIL

#include <utility>
#include <algorithm>
#include <vector>
#include <cmath>

using namespace std;

#include "const.h"
#include "debug.h"

/*************
 * CHECK/TEST macros
 */

#define CHECK(x) while(1) { if(!(x)) { dprintf("Error at %s:%d - %s\n", __FILE__, __LINE__, #x); *(int*)0 = 0; } break; }
#define TEST(x) CHECK(x)
#define printf FAILURE

/*************
 * Text processing
 */

#ifdef printf
#define PFDEFINED
#undef printf
#endif

string StringPrintf( const char *bort, ... ) __attribute__((format(printf,1,2)));

#ifdef PFDEFINED
#define printf FAILURE
#undef PFDEFINED
#endif

/*************
 * User input
 */
 
class Button {
public:
    bool down;
    bool up;
    bool push;
    bool release;
    bool repeat;
    int dur;
    int sincerep;
    
    void newState(bool pushed);
    void newState(const Button &other);

    Button();
};

class Controller {
public:
    float x;
    float y;
    Button u;
    Button d;
    Button l;
    Button r;
    vector<Button> keys;

    void newState(const Controller &nst);

};

class Keystates {
public:
    Button u,d,l,r,f;

    void newState(const Keystates &nst);

	Keystates();
};

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

inline Float2 operator+(const Float2 &lhs, const Float2 &rhs) {
    return Float2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline Float2 operator*(const Float2 &lhs, float rhs) {
    return Float2(lhs.x * rhs, lhs.y * rhs);
}

class Float4 {
public:
	float sx, sy, ex, ey;
	Float4() { };
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
}

inline bool operator==( const Float4 &lhs, const Float4 &rhs ) {
	return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

/*************
 * Fast sin/cos
 */

#define SIN_TABLE_SIZE 180
extern float sin_table[ SIN_TABLE_SIZE + 1 ];

inline float dsin( float in ) {
	return sin_table[ int( in * ( 2 * SIN_TABLE_SIZE / PI ) + 0.5f ) ];
}

inline float fsin( float in ) {
    if(in < 0 || in >= PI * 5 / 2) {
        in = fmod(in, PI * 2);
        if(in < 0)
            in = in + PI * 2;
        return fsin(in);
    }
    
    CHECK(in >= 0);
    if( in < PI / 2 ) {
		return dsin( in );
	} else if( in < PI ) {
		return dsin( PI - in );
	} else if( in < PI * 3 / 2 ) {
		return -dsin( in - PI );
	} else if( in < PI * 2 ) {
		return -dsin( PI * 2 - in );
	} else if( in < PI * 5 / 2 ) {
		return dsin( in - PI * 2 );
	} else {
        CHECK(0);
	}
}
inline float fcos( float in ) {
	return fsin( in + PI / 2 );
}

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
inline float linelineintersectpos( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 ) {
	float denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
	float ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	float ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	if( ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 )
		return ua;
	else
		return 2.f;
}
inline float linelineintersectpos( const Float4 &lhs, const Float4 &rhs ) {
	return linelineintersectpos( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}
inline int whichSide( const Float4 &f4, const pair< float, float > &pta ) {
    float ax = f4.ex - f4.sx;
    float ay = f4.ey - f4.sy;
    float bx = pta.first - f4.sx;
    float by = pta.second - f4.sy;
    swap(ax, ay);
    ax *= -1;
    float rv = ax * bx + ay * by;
    if( rv < 0 ) return -1;
    else if( rv > 0 ) return 1;
    else return 0;
}

inline Float2 angle( float ang ) {
    return Float2(fsin(ang), fcos(ang));
}

inline bool isinside(const Float4 &lhs, const Float2 &rhs) {
    return !(rhs.x < lhs.sx || rhs.x >= lhs.ex || rhs.y < lhs.sy || rhs.y >= lhs.ey);
}

inline Float4 boxaround(const Float2 &lhs, float radius) {
    return Float4(lhs.x - radius, lhs.y - radius, lhs.x + radius, lhs.y + radius);
}

/*************
 * Math
 */

inline Float4 lerp( const Float4 &start, const Float4 &delta, float time ) {
    return Float4( start.sx + delta.sx * time, start.sy + delta.sy * time, start.ex + delta.ex * time, start.ey + delta.ey * time );
}

inline float frand() {
    return rand() / ( RAND_MAX + 1.0 );
}

inline int round(float in) {
    return int(floor(in + 0.5));
}

#endif
