
#include "util.h"

#include <assert.h>
#include <numeric>
using namespace std;

Float4::Float4() { };
Float4::Float4( float in_sx, float in_sy, float in_ex, float in_ey ) {
	sx = in_sx; sy = in_sy; ex = in_ex; ey = in_ey; };
Float4 Float4::normalize() const {
	return Float4( min( sx, ex ), min( sy, ey ), max( sx, ex ), max( sy, ey ) ); };
bool Float4::rectIntersects( const Float4 &rhs ) const {
	assert( normalize() == *this );
	assert( rhs.normalize() == rhs );
	if( ex <= rhs.sx || rhs.ex <= sx || ey <= rhs.sy || rhs.ey <= sy )
		return false;
	return true;
}

bool operator==( const Float4 &lhs, const Float4 &rhs ) {
	return lhs.sx == rhs.sx && lhs.sy == rhs.sy && lhs.ex == rhs.ex && lhs.ey == rhs.ey;
}
