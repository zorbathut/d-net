#ifndef DNET_UTIL
#define DNET_UTIL

class Float4 {
public:
	float sx, sy, ex, ey;
	Float4();
	Float4( float sx, float sy, float ex, float ey );

	Float4 normalize() const;
	bool rectIntersects( const Float4 &rhs ) const;
};

bool operator==( const Float4 &lhs, const Float4 &rhs );

#endif
