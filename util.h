#ifndef DNET_UTIL
#define DNET_UTIL

class Float4 {
public:
	float sx, sy, ex, ey;
	Float4();
	Float4( float sx, float sy, float ex, float ey );

	inline Float4 normalize() const {
		Float4 cp = *this;
		float tmp;
		if( cp.sx > cp.ex ) {
			tmp = cp.sx;
			cp.sx = cp.ex;
			cp.ex = tmp;
		}
		if( cp.sx > cp.ex ) {
			tmp = cp.sy;
			cp.sy = cp.ey;
			cp.ey = tmp;
		}
		return cp;
	}
};

bool operator==( const Float4 &lhs, const Float4 &rhs );

bool rectrectintersect( const Float4 &lhs, const Float4 &rhs );
bool linelineintersect( const Float4 &lhs, const Float4 &rhs );
bool linelineintersect( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 );

#define assert2(x)

#endif
