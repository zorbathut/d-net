
#include "collide.h"

#include <assert.h>

#include "debug.h"

bool linelineintersect( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 ) {
	float denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
	float ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	float ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}

void Collider::startGroup() {
}; // the deep dark secret
int Collider::endGroup() {
	groups.push_back( verts.size() );
	active.push_back( true );
	return active.size() - 1;
};

void Collider::add( float sx, float sy, float ex, float ey ) {
	verts.push_back( sx );
	verts.push_back( sy );
	verts.push_back( ex );
	verts.push_back( ey );
};

void Collider::disableGroup( int group ) {
	assert( group >= 0 && group < active.size() );
	active[ group ] = false;
};
void Collider::enableGroup( int group ) {
	assert( group >= 0 && group < active.size() );
	active[ group ] = true;
};

bool Collider::test( float sx, float sy, float ex, float ey ) const {
	for( int i = 0; i < active.size(); i++ )
		if( active[ i ] )
			for( int j = groups[ i ]; j < groups[ i + 1 ]; j += 4 )
				if( linelineintersect( verts[ j ], verts[ j + 1 ], verts[ j + 2 ], verts[ j + 3 ], sx, sy, ex, ey ) )
					return true;
	return false;
};

Collider::Collider() {
	groups.push_back( 0 );
};
