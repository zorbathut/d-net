
#include "util.h"

#include <assert.h>
#include <numeric>
using namespace std;

Float4::Float4() { };
Float4::Float4( float in_sx, float in_sy, float in_ex, float in_ey ) {
	sx = in_sx; sy = in_sy; ex = in_ex; ey = in_ey; };

bool operator==( const Float4 &lhs, const Float4 &rhs ) {
	return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}

bool rectrectintersect( const Float4 &lhs, const Float4 &rhs ) {
	assert2( lhs.normalize() == lhs );
	assert2( rhs.normalize() == rhs );
	if( lhs.ex <= rhs.sx || rhs.ex <= lhs.sx || lhs.ey <= rhs.sy || rhs.ey <= lhs.sy )
		return false;
	return true;
}

bool linelineintersect( const Float4 &lhs, const Float4 &rhs ) {
	return linelineintersect( lhs.sx, lhs.sy, lhs.ex, lhs.ey, rhs.sx, rhs.sy, rhs.ex, rhs.ey );
}

bool linelineintersect( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 ) {
	float denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
	float ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	float ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}
