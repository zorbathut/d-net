
#include "collide.h"

#include <algorithm>
#include <assert.h>
using namespace std;

#include "debug.h"

Collide::Collide() { };
Collide::Collide( const Float4 &in_line, int in_sid ) : line( in_line ) {
	sid = in_sid; };

Quad::Quad() { quads = NULL; sludge = false; }
Quad::Quad( const Float4 &dim ) : range( dim ) {
	quads = NULL; sludge = false; };
Quad::~Quad() {
	delete [] quads; }

void Collider::startGroup() {
	newGroup.clear();
};
int Collider::endGroup() {

	groups.push_back( verts.size() );
	active.push_back( true );

	int groupid = active.size() - 1;
	vector< Collide > *nv = new vector< Collide >;
	nv->swap( newGroup );
	collides.push_back( nv );
	vector< Collide > &cols = *nv;
	//dprintf( "End/adding %d items\n", cols.size() );
	for( int i = 0; i < cols.size(); i++ ) {
		cols[ i ].group = groupid;
		quadAdd( cols[ i ].line.normalize(), &cols[ i ], quad );
	}
	//dprintf( "Add complete\n", cols.size() );

	return groupid;
};

void Collider::add( float sx, float sy, float ex, float ey ) {
	//dprintf( "Adding %f, %f, %f, %f\n", sx, sy, ex, ey );
	verts.push_back( sx );
	verts.push_back( sy );
	verts.push_back( ex );
	verts.push_back( ey );
	newGroup.push_back( Collide( Float4( sx, sy, ex, ey ), newGroup.size() ) );
};

void Collider::disableGroup( int group ) {
	assert( group >= 0 && group < active.size() );
	active[ group ] = false;
};
void Collider::enableGroup( int group ) {
	assert( group >= 0 && group < active.size() );
	active[ group ] = true;
};
void Collider::deleteGroup( int group ) {
	assert( group >= 0 && group < active.size() );
	active[ group ] = false;
	for( int i = 0; i < collides[ group ]->size(); i++ )
		quadRemove( (*collides[ group ])[ i ].line.normalize(), &(*collides[ group ])[ i ], quad );
};

bool Collider::test( float sx, float sy, float ex, float ey ) const {
	bool res = false;
	for( int i = 0; i < active.size() && !res; i++ )
		if( active[ i ] )
			for( int j = groups[ i ]; j < groups[ i + 1 ]; j += 4 )
				if( linelineintersect( verts[ j ], verts[ j + 1 ], verts[ j + 2 ], verts[ j + 3 ], sx, sy, ex, ey ) ) {
					res = true;
					break;
				}
	//dprintf( "Entering qt\n" );
	bool alterres = quadTest( Float4( sx, sy, ex, ey ), Float4( sx, sy, ex, ey ).normalize(), quad );
	//dprintf( "Leaving qt\n" );
	if( res != alterres )
		dprintf( "Mismatch!\n" );
	assert( res == alterres );
	return res;
};

bool Collider::quadTest( const Float4 &line, const Float4 &range, const Quad *node ) const {
	if( rectrectintersect( range, node->range ) ) {
		if( node->quads ) {
			//dprintf( "QTL\n" );
			for( int i = 0; i < 4; i++ )
				if( quadTest( line, range, &node->quads[ i ] ) )
					return true;
			//dprintf( "EQTL\n" );
		} else {
			//dprintf( "LIN\n" );
			for( int i = 0; i < node->lines.size(); i++ )
				if( active[ node->lines[ i ]->group] && linelineintersect( line, node->lines[ i ]->line ) )
					return true;
			//dprintf( "ELIN\n" );
		}
	}
	return false;
}

#define MAX_LEAF_SIZE	16
#define LEAF_SLUDGE_TOTAL (MAX_LEAF_SIZE*2)
//#define LEAF_SLUDGE_THRESH (MAX_LEAF_SIZE*3/4)
#define LEAF_SLUDGE_UNSLUDGE (MAX_LEAF_SIZE*3/4)

void Collider::quadAdd( const Float4 &range, Collide *ptr, Quad *quad ) {
//	depth++;
	if( rectrectintersect( range, quad->range ) ) {
		//dprintf( "Entering %d\n", depth );
		if( !quad->quads && !quad->sludge && quad->lines.size() >= MAX_LEAF_SIZE ) {
			//dprintf( "Splitz0ring %d\n", depth );
			// SPLITZ0R
			quad->quads = new Quad[ 4 ];
			quad->quads[ 0 ].range = Float4( quad->range.sx, quad->range.sy, ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2 );
			quad->quads[ 1 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, quad->range.sy, quad->range.ex, ( quad->range.sy + quad->range.ey ) / 2 );
			quad->quads[ 2 ].range = Float4( quad->range.sx, ( quad->range.sy + quad->range.ey ) / 2, ( quad->range.sx + quad->range.ex ) / 2, quad->range.ey );
			quad->quads[ 3 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2, quad->range.ex, quad->range.ey );
			for( int i = 0; i < quad->lines.size(); i++ )
				quadAdd( quad->lines[ i ]->line.normalize(), quad->lines[ i ], quad );
			{
				int sludgetot = 0;
				for( int i = 0; i < 4; i++ ) {
					sludgetot += quad->quads[ i ].lines.size();
//					if( quad->quads[ i ].lines.size() >= LEAF_SLUDGE_THRESH )
//						quad->sludge = true;
				}
				if( sludgetot >= LEAF_SLUDGE_TOTAL )
					quad->sludge = true;
			}
			if( quad->sludge ) {
				delete [] quad->quads;
				quad->quads = NULL;
				dprintf( "WARNING: Sludge node generated!\n" );
			} else {
				quad->lines.clear();
			}
		}
		if( quad->quads ) {
			//dprintf( "Subadding %d\n", depth );
			for( int i = 0; i < 4; i++ )
				quadAdd( range, ptr, &quad->quads[ i ] );
		} else {
			//dprintf( "Adding %d\n", depth );
			//dprintf( "Adding %08x to %08x\n", ptr, quad );
			quad->lines.push_back( ptr );
		}
		//dprintf( "Done %d\n", depth );
	}
//	depth--;
};

void Collider::quadRemove( const Float4 &range, Collide *ptr, Quad *quad ) {
	if( rectrectintersect( range, quad->range ) ) {
		if( quad->quads ) {
			for( int i = 0; i < 4; i++ )
				quadRemove( range, ptr, &quad->quads[ i ] );
		} else {
			//dprintf( "Removing %08x from %08x\n", ptr, quad );
			vector< Collide * >::iterator itr = find( quad->lines.begin(), quad->lines.end(), ptr );
			assert( itr != quad->lines.end() );
			quad->lines.erase( itr );
			if( quad->sludge && quad->lines.size() < LEAF_SLUDGE_UNSLUDGE ) {
				quad->sludge = false;
				dprintf( "Sludge node reclaimed!\n" );
			}
		}
	}
}


Collider::Collider() { quad = NULL; };
Collider::Collider( float sx, float sy, float ex, float ey ) {
	groups.push_back( 0 );
	quad = new Quad( Float4( sx, sy, ex, ey ) );
};
void Collider::reinit( float sx, float sy, float ex, float ey ) {

	newGroup.clear();
	for( int i = 0; i < collides.size(); i++ )
		delete collides[ i ];
	collides.clear();
	verts.clear();
	groups.clear();
	active.clear();
	delete quad;

	groups.push_back( 0 );
	quad = new Quad( Float4( sx, sy, ex, ey ) );
		
}
Collider::~Collider() {
	delete quad;
	for( int i = 0; i < collides.size(); i++ )
		delete collides[ i ];
};

#include "gfx.h"
void quadRender( Quad *x ) {
	if( x->quads ) {
		for( int i = 0; i < 4; i++ )
			quadRender( &x->quads[ i ] );
	} else {
		drawRect( x->range, 0.4 );
	}
}

void Collider::render() const {
	setColor( 0, 0, 1 );
	quadRender( quad );
}