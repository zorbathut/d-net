
#include "collide.h"

#include <algorithm>
#include <assert.h>
using namespace std;

#include "debug.h"
#include "rng.h"

Collide::Collide() { };
Collide::Collide( const Float4 &in_line, int in_sid ) : line( in_line ) {
	sid = in_sid; };

Quad::Quad() {
	quads = NULL;
	sludge = false;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	r = unsyncRand() * 1.f / RNG_MAX;
	g = unsyncRand() * 1.f / RNG_MAX;
	b = unsyncRand() * 1.f / RNG_MAX;
	dprintf( "this is %f,%f,%f\n", r, g, b );
#endif
}
Quad::Quad( const Float4 &dim ) : range( dim ) {
	quads = NULL;
	sludge = false;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	r = unsyncRand() * 1.f / RNG_MAX;
	g = unsyncRand() * 1.f / RNG_MAX;
	b = unsyncRand() * 1.f / RNG_MAX;
	dprintf( "this is %f,%f,%f\n", r, g, b );
#endif
};
Quad::~Quad() {
	delete [] quads; }

void Collider::startGroup() {
	newGroup.clear();
};
int Collider::endGroup() {

	int groupid;
	if( cleared.size() ) {
		groupid = cleared[ cleared.size() - 1 ];
		//dprintf( "Reclaimed group %d, %d\n", groupid, collides[ groupid ]->size() );
		assert( active[ groupid ] == false );
		assert( collides[ groupid ]->size() == 0 );
		cleared.pop_back();
		active[ groupid ] = true;
	} else {
		groupid = active.size();
		active.push_back( true );
		collides.push_back( new vector< Collide >() );
	}

	vector< Collide > &cols = *collides[ groupid ];
	cols.swap( newGroup );
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
	collides[ group ]->clear();
	//dprintf( "Wiped group %d, %d\n", group, collides[ group ]->size() );
	cleared.push_back( group );
};

bool Collider::test( float sx, float sy, float ex, float ey ) const {
	return quadTest( Float4( sx, sy, ex, ey ), Float4( sx, sy, ex, ey ).normalize(), quad );
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
			//dprintf( "Splitz0ring\n" );
			// SPLITZ0R
			quad->quads = new Quad[ 4 ];
			quad->quads[ 0 ].range = Float4( quad->range.sx, quad->range.sy, ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2 );
			quad->quads[ 1 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, quad->range.sy, quad->range.ex, ( quad->range.sy + quad->range.ey ) / 2 );
			quad->quads[ 2 ].range = Float4( quad->range.sx, ( quad->range.sy + quad->range.ey ) / 2, ( quad->range.sx + quad->range.ex ) / 2, quad->range.ey );
			quad->quads[ 3 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2, quad->range.ex, quad->range.ey );
			//dprintf( "Readding %d items\n", quad->lines.size() );
			for( int i = 0; i < quad->lines.size(); i++ ) {
				//dprintf( "Start %08x\n", quad->lines[ i ] );
				quadAdd( quad->lines[ i ]->line.normalize(), quad->lines[ i ], quad );
			}
			//dprintf( "Done\n" );
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
				quadAdd( range, ptr, &(quad->quads[ i ]) );
				#if 0
					#ifndef NDEBUG
						int pts = 0;
						for( int i = 0; i < 4; i++ )
							pts += count( quad->quads[ i ].lines.begin(), quad->quads[ i ].lines.end(), ptr );
						dprintf( "pts is %d\n", pts );
						if( pts == 0 ) {
							dprintf( "%f, %f, %f, %f\n", range.sx, range.sy, range.ex, range.ey );
							dprintf( "%f, %f, %f, %f\n", quad->range.sx, quad->range.sy, quad->range.ex, quad->range.ey );
							dprintf( "tc is %d\n", rectrectintersect( quad->range, range ) );
							for( int i = 0; i < 4; i++ ) {
								dprintf( "%f, %f, %f, %f\n", quad->quads[ i ].range.sx, quad->quads[ i ].range.sy, quad->quads[ i ].range.ex, quad->quads[ i ].range.ey );
								dprintf( "%d is %d\n", i, rectrectintersect( quad->quads[ i ].range, range ) );
							}
						}
						assert( pts );
					#endif
				#endif
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
	quad = new Quad( Float4( sx, sy, ex, ey ) );
};
void Collider::reinit( float sx, float sy, float ex, float ey ) {

	newGroup.clear();
	for( int i = 0; i < collides.size(); i++ )
		delete collides[ i ];
	collides.clear();
	active.clear();
	cleared.clear();
	delete quad;

	quad = new Quad( Float4( sx, sy, ex, ey ) );
		
}
Collider::~Collider() {
	delete quad;
	for( int i = 0; i < collides.size(); i++ )
		delete collides[ i ];
};

#ifdef ENABLE_COLLIDE_DEBUG_VIS

#include "gfx.h"
void quadRender( Quad *x ) {
	setColor( x->r, x->g, x->b );
	if( x->quads ) {
		for( int i = 0; i < 4; i++ )
			quadRender( &x->quads[ i ] );
	} else {
		drawRect( Float4( x->range.sx + 1, x->range.sy + 1, x->range.ex - 1, x->range.ey - 1 ), 0.4 );
		//setColor( x->r / 2, x->g / 2, x->b / 2 );
		//for( int i = 0; i < x->lines.size(); i++ )
			//drawRect( x->lines[ i ]->line.normalize(), 0.3 );
	}
}

void Collider::render() const {
	//setColor( 0, 0, 1 );
	//quadRender( quad );
}

#else

void Collider::render() const {
}

#endif
