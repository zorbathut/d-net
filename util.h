#ifndef DNET_UTIL
#define DNET_UTIL

#include "const.h"

#include <assert.h>

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

#define assert2(x) assert(x)

inline bool operator==( const Float4 &lhs, const Float4 &rhs ) {
	return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

inline bool rectrectintersect( const Float4 &lhs, const Float4 &rhs ) {
	assert2( lhs.isNormalized() );
	assert2( rhs.isNormalized() );
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

#define SIN_TABLE_SIZE 90

extern float sin_table[ SIN_TABLE_SIZE + 1 ];

inline float dsin( float in ) {
	return sin_table[ int( in * ( 2 * SIN_TABLE_SIZE / PI ) + 0.5f ) ];
}

inline float fsin( float in ) {
	assert( in >= 0 );
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
		assert( 0 );
	}
}
inline float fcos( float in ) {
	return fsin( in + PI / 2 );
}

#endif
