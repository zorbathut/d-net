#if 1
#include "collide.h"

#include <algorithm>
#include <cmath>

using namespace std;

#include "debug.h"
#include "rng.h"
#include "util.h"

const float NOCOLLIDE = -1e8;

pair< float, float > getLineCollision( const Float4 &linepos, const Float4 &linevel, const Float4 &ptposvel ) {
	float x1d = linepos.sx;
	float y1d = linepos.sy;
	float x2d = linepos.ex;
	float y2d = linepos.ey;
	float x1v = linevel.sx;
	float y1v = linevel.sy;
	float x2v = linevel.ex;
	float y2v = linevel.ey;
	float x3d = ptposvel.sx;
	float y3d = ptposvel.sy;
	float x3v = ptposvel.ex;
	float y3v = ptposvel.ey;
	float a = -x3v*y1v-x1v*y2v+x3v*y2v+x2v*y1v+x1v*y3v-x2v*y3v;
	float b = +x2v*y1d-x3v*y1d+x2d*y1v-x3d*y1v-x1v*y2d+x3v*y2d-x1d*y2v+x3d*y2v+x1v*y3d-x2v*y3d+x1d*y3v-x2d*y3v;
	float c = +x2d*y1d-x3d*y1d-x1d*y2d+x3d*y2d+x1d*y3d-x2d*y3d;
	//dprintf( "a is %f, b is %f, c is %f\n", a, b, c );
	float sqrii = b * b - 4 * a * c;
	//dprintf( "sqrii is %f\n", sqrii );
	float a2 = 2 * a;
	//dprintf( "a2 is %f\n", a2 );
	if( sqrii < 0 )
		return make_pair( NOCOLLIDE, NOCOLLIDE );
	pair< float, float > rv;
	if( a2 == 0 ) {
		if( b == 0 ) {
			return make_pair( NOCOLLIDE, NOCOLLIDE );
		}
		rv.first = -c / b;
		rv.second = NOCOLLIDE;
	} else {
		float sqrit = sqrt( sqrii );
		rv.first = ( -b + sqrit ) / a2;
		rv.second = ( -b - sqrit ) / a2;
	}
	{
		if( rv.first != NOCOLLIDE ) {
			assert( fabs( a * rv.first * rv.first + b * rv.first + c ) < 1e6 );
			//dprintf( "debugtest: %f resolves to %f\n", rv.first, a * rv.first * rv.first + b * rv.first + c );
		}
		if( rv.second != NOCOLLIDE ) {
			assert( fabs( a * rv.second * rv.second + b * rv.second + c ) < 1e6 );
			//dprintf( "debugtest: %f resolves to %f\n", rv.second, a * rv.second * rv.second + b * rv.second + c );
		}
	}
	return rv;
}

float sqr( float x ) {
	return x * x;
}

float getu( const Float4 &linepos, const Float4 &linevel, const Float4 &ptposvel, float t ) {
	float x1 = linepos.sx + linevel.sx * t;
	float y1 = linepos.sy + linevel.sy * t;
	float x2 = linepos.ex + linevel.ex * t;
	float y2 = linepos.ey + linevel.ey * t;
	float x3 = ptposvel.sx + ptposvel.ex * t;
	float y3 = ptposvel.sy + ptposvel.ey * t;
	return ( ( x3 - x1 ) * ( x2 - x1 ) + ( y3 - y1 ) * ( y2 - y1 ) ) / ( sqr( x2 - x1 ) + sqr( y2 - y1 ) );
}

float getCollision( const Float4 &l1p, const Float4 &l1v, const Float4 &l2p, const Float4 &l2v, float curtime ) {
	float cBc = NOCOLLIDE;
	Float4 temp;
	for( int i = 0; i < 4; i++ ) {
		const Float4 *linepos;
		const Float4 *linevel;
		const Float4 *ptposvel;
		switch( i ) {
			case 0:
				linepos = &l1p;
				linevel = &l1v;
				temp = Float4( l2p.sx, l2p.sy, l2v.sx, l2v.sy );
				ptposvel = &temp;
				break;
			case 1:
				linepos = &l1p;
				linevel = &l1v;
				temp = Float4( l2p.ex, l2p.ey, l2v.ex, l2v.ey );
				ptposvel = &temp;
				break;
			case 2:
				linepos = &l2p;
				linevel = &l2v;
				temp = Float4( l1p.sx, l1p.sy, l1v.sx, l1v.sy );
				ptposvel = &temp;
				break;
			case 3:
				linepos = &l2p;
				linevel = &l2v;
				temp = Float4( l1p.ex, l1p.ey, l1v.ex, l1v.ey );
				ptposvel = &temp;
				break;
			default:
				assert( 0 );
		}
        pair< float, float > tbv = getLineCollision( *linepos, *linevel, *ptposvel );
		for( int j = 0; j < 2; j++ ) {
			float tt;
			if( j ) {
				tt = tbv.second;
			} else {
				tt = tbv.first;
			}
			//if( verbosified && tt != NOCOLLIDE )
				//dprintf( "%d, %d is %f\n", i, j, tt );
			if( tt < 0 || tt > 1 || ( cBc != NOCOLLIDE && tt > cBc ) || tt < curtime )
				continue;
			float u = getu( *linepos, *linevel, *ptposvel, tt );
			if( u < 0 || u > 1 )
				continue;
			cBc = tt;
		}
	}
	return cBc;
}

void Collider::reset() {
	assert( state == 0 );
	items.clear();
	ctime = 0;
}

void Collider::startToken( int toki ) {
	assert( state == 1 && curpush != -1 );
    curtoken = toki;
}
void Collider::token( const Float4 &line, const Float4 &direction ) {
	assert( state == 1 && curpush != -1 && curtoken != -1 );
    assert( ctime == 0 || ( direction.sx == 0 && direction.sy == 0 && direction.ex == 0 && direction.ey == 0 ) );
	items[ curpush ].push_back( make_pair( curtoken, make_pair( line, direction ) ) );
}

void Collider::createGroup() {
	assert( state == 0 && curpush == -1 && curtoken == -1 );
	state = 1;
    curpush = items.size();
	items.push_back( vector< pair< int, pair< Float4, Float4 > > >() );
}
int Collider::endCreateGroup() {
	assert( state == 1 && curpush != -1 );
	state = 0;
    curpush = -1;
    curtoken = -1;
	return items.size() - 1;
}

void Collider::clearGroup( int gid ) {
    assert( state == 0 && curpush == -1 && curtoken == -1 );
    assert( gid >= 0 && gid < items.size() );
    items[ gid ].clear();
}

void Collider::addThingsToGroup( int gid ) {
    assert( state == 0 && curpush == -1 && curtoken == -1 );
    assert( gid >= 0 && gid < items.size() );
    state = 1;
    curpush = gid;
}

void Collider::endAddThingsToGroup() {
    assert( state == 1 && curpush != -1 );
    state = 0;
    curpush = -1;
    curtoken = -1;
}

bool Collider::doProcess() {
	assert( state == 0 );
	float firstintersect = 2.0;
	for( int x = 0; x < items.size(); x++ ) {
		for( int y = x + 1; y < items.size(); y++ ) {
			if( x == 0 && y == 1 && frameNumber % 30 == 0 )
				verbosified = true;
			for( int xa = 0; xa < items[ x ].size(); xa++ ) {
				for( int ya = 0; ya < items[ y ].size(); ya++ ) {
					float tcol = getCollision( items[ x ][ xa ].second.first, items[ x ][ xa ].second.second, items[ y ][ ya ].second.first, items[ y ][ ya ].second.second, ctime );
					if( tcol == NOCOLLIDE )
						continue;
					//dprintf( "%d,%d collides with %d%f, %f\n", ctime, tcol );
					assert( tcol >= ctime );
					if( tcol < firstintersect ) {
						firstintersect = tcol;
						lhs.first = x;
						lhs.second = xa;
						rhs.first = y;
						rhs.second = ya;
					}
				}
			}
			verbosified = false;
		}
	}
	ctime = firstintersect;
	return firstintersect < 2.0;
}

float Collider::getCurrentTimestamp() {
	assert( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return ctime;
}
void Collider::setCurrentTimestamp( float ntime ) {
	assert( state == 0 && ntime <= 1.0 && ntime >= 0.0 );
	ctime = ntime;
}

pair< int, int > Collider::getLhs() {
	assert( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return lhs;
}
pair< int, int > Collider::getRhs() {
	assert( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return rhs;
}

void Collider::flagAsMoved( int group ) { };

bool Collider::testCollideAgainst( int active, int start, int end, float time ) {
    assert( start >= 0 && start < items.size() );
    assert( end >= 0 && end < items.size() );
    assert( start <= end );
    assert( active >= 0 && active < items.size() );
    for( int x = start; x < end; x++ ) {
        if( x == active )
            continue;
        for( int y = 0; y < items[ x ].size(); y++ ) {
            for( int k = 0; k < items[ active ].size(); k++ ) {
                if( linelineintersect( lerp( items[ x ][ y ].second.first, items[ x ][ y ].second.second, time ), lerp( items[ active ][ k ].second.first, items[ active ][ k ].second.second, time ) ) ) {
                    //dprintf( "%d/%d with %d/%d\n", active, k, x, y );
                    return true;
                }
            }
        }
    }
    return false;
}

Collider::Collider() { state = 0; ctime = 0; curpush = -1; curtoken = -1; };
//Collide::Collide( const Float4 &in_line, int in_sid ) : line( in_line ) {
	//sid = in_sid; };
Collider::~Collider() { };

void Collider::render() const { };

#else
/*
#include "collide.h"

#include <algorithm>
#include <cmath>

using namespace std;

#include "debug.h"
#include "rng.h"

const float NOCOLLIDE = -1e8;

pair< float, float > getPartCollision( const Float4 &linepos, const Float4 &linevel, const Float4 &ptposvel ) {
	float x1d = linepos.sx;
	float y1d = linepos.sy;
	float x2d = linepos.ex;
	float y2d = linepos.ey;
	float x1v = linevel.sx;
	float y1v = linevel.sy;
	float x2v = linevel.ex;
	float y2v = linevel.ey;
	float x3d = ptposvel.sx;
	float y3d = ptposvel.sy;
	float x3v = ptposvel.ex;
	float y3v = ptposvel.ey;
	float b = -x2v*y1d + x3v*y1d - x2d*y1v + x3d*y1v + x1v*y2d - x3v*y2d + x1d*y2v - x3d*y2v - x1v*y3d + x2v*y3d - x1d*y3v + x2d*y3v;
	float a = -x2v*y1v + x3v*y1v + x1v*y2v - x3v*y2v - x1v*y3v + x2v*y3v;
	float c = -x2d*y1d + x3d*y1d + x1d*y2d - x3d*y2d - x1d*y3d + x2d*y3d;
	float sqrii = b * b - 4 * a * c;
	float a2 = 2 * a;
	if( sqrii < 0 )
		return make_pair( NOCOLLIDE, NOCOLLIDE );
	float sqrit = sqrt( sqrii );
	return make_pair( ( -b + sqrii ) / a2, ( -b - sqrii ) / a2 );
}

float sqr( float x ) {
	return x * x;
}

float getu( const Float4 &linepos, const Float4 &linevel, const Float4 &ptposvel, float t ) {
	float x1 = linepos.sx + linevel.sx * t;
	float y1 = linepos.sy + linevel.sy * t;
	float x2 = linepos.ex + linevel.ex * t;
	float y2 = linepos.ey + linevel.ey * t;
	float x3 = ptposvel.sx + ptposvel.ex * t;
	float y3 = ptposvel.sy + ptposvel.ey * t;
	return ( ( x3 - x1 ) * ( x2 - x1 ) + ( y3 - y1 ) * ( y2 - y1 ) ) / ( sqr( x2 - x1 ) + sqr( y2 - y1 ) );
}

float getCollision( const Float4 &l1p, const Float4 &l1v, const Float4 &l2p, const Float4 &l2v ) {
	dprintf( "Start GC\n" );
	float cBc = NOCOLLIDE;
	Float4 temp;
	for( int i = 0; i < 4; i++ ) {
		const Float4 *linepos;
		const Float4 *linevel;
		const Float4 *ptposvel;
		switch( i ) {
			case 0:
				linepos = &l1p;
				linevel = &l1v;
				temp = Float4( l2p.sx, l2p.sy, l2v.sx, l2v.sy );
				ptposvel = &temp;
				break;
			case 1:
				linepos = &l1p;
				linevel = &l1v;
				temp = Float4( l2p.ex, l2p.ey, l2v.ex, l2v.ey );
				ptposvel = &temp;
				break;
			case 2:
				linepos = &l2p;
				linevel = &l2v;
				temp = Float4( l1p.sx, l1p.sy, l1v.sx, l1v.sy );
				ptposvel = &temp;
				break;
			case 3:
				linepos = &l2p;
				linevel = &l2v;
				temp = Float4( l1p.ex, l1p.ey, l1v.ex, l1v.ey );
				ptposvel = &temp;
				break;
			default:
				assert( 0 );
		}
        pair< float, float > tbv = getPartCollision( *linepos, *linevel, *ptposvel );
		for( int j = 0; j < 2; j++ ) {
			float tt;
			if( j ) {
				tt = tbv.second;
			} else {
				tt = tbv.first;
			}
			dprintf( "%d, %d is %f\n", i, j, tt );
			if( tt < 0 || tt > 1 || ( cBc != NOCOLLIDE && tt > cBc ) )
				continue;
			float u = getu( *linepos, *linevel, *ptposvel, tt );
			if( u < 0 || u > 1 )
				continue;
			cBc = tt;
		}
	}
	return cBc;
}

Collide::Collide() { };
Collide::Collide( const Float4 &in_line, int in_sid ) : line( in_line ) {
	sid = in_sid; };

Quad::Quad() {
	quads = NULL;
	sludge = false;
	node = false;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	r = unsyncRand() * 1.f / RNG_MAX;
	g = unsyncRand() * 1.f / RNG_MAX;
	b = unsyncRand() * 1.f / RNG_MAX;
#endif
}
Quad::Quad( const Float4 &dim ) : range( dim ) {
	quads = NULL;
	sludge = false;
	node = false;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	r = unsyncRand() * 1.f / RNG_MAX;
	g = unsyncRand() * 1.f / RNG_MAX;
	b = unsyncRand() * 1.f / RNG_MAX;
#endif
};
Quad::~Quad() {
	delete [] quads; }

/*
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
};*/
/*
void Collider::add( float sx, float sy, float ex, float ey ) {
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
	cleared.push_back( group );
};

bool Collider::test( float sx, float sy, float ex, float ey ) const {
	return quadTest( Float4( sx, sy, ex, ey ), Float4( sx, sy, ex, ey ).normalize(), quad );
};

pair< float, int > Collider::getImpact( float sx, float sy, float ex, float ey ) const {
	return quadImpact( Float4( sx, sy, ex, ey ), Float4( sx, sy, ex, ey ).normalize(), quad );
};

bool Collider::quadTest( const Float4 &line, const Float4 &range, const Quad *node ) const {
	if( rectrectintersect( range, node->range ) ) {
		if( node->node ) {
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

pair< float, int > Collider::quadImpact( const Float4 &line, const Float4 &range, const Quad *node ) const {
	pair< float, int > rv = make_pair( 2.0f, -1 );
	if( rectrectintersect( range, node->range ) ) {
		if( node->node ) {
			for( int i = 0; i < 4; i++ ) {
				pair< float, int > nrv = quadImpact( line, range, &node->quads[ i ] );
				if( nrv < rv )
					rv = nrv;
			}
		} else {
			for( int i = 0; i < node->lines.size(); i++ )
				if( active[ node->lines[ i ]->group] ) {
					pair< float, int > nrv = make_pair( linelineintersectpos( line, node->lines[ i ]->line ), node->lines[ i ]->group );
					if( nrv < rv )
						rv = nrv;
				}
		}
	}
	return rv;
}



#define MAX_LEAF_SIZE	16
#define LEAF_COLLAPSE_THRESH 12
#define LEAF_SLUDGE_TOTAL (MAX_LEAF_SIZE*2)
//#define LEAF_SLUDGE_THRESH (MAX_LEAF_SIZE*3/4)
#define LEAF_SLUDGE_UNSLUDGE (MAX_LEAF_SIZE*3/4)

void Collider::quadAdd( const Float4 &range, Collide *ptr, Quad *quad ) {
	if( rectrectintersect( range, quad->range ) ) {
		if( !quad->node && !quad->sludge && quad->lines.size() >= MAX_LEAF_SIZE ) {
			//dprintf( "Splitz0ring\n" );
			// SPLITZ0R
			if( !quad->quads ) {
				quad->quads = new Quad[ 4 ];
				quad->quads[ 0 ].range = Float4( quad->range.sx, quad->range.sy, ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2 );
				quad->quads[ 1 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, quad->range.sy, quad->range.ex, ( quad->range.sy + quad->range.ey ) / 2 );
				quad->quads[ 2 ].range = Float4( quad->range.sx, ( quad->range.sy + quad->range.ey ) / 2, ( quad->range.sx + quad->range.ex ) / 2, quad->range.ey );
				quad->quads[ 3 ].range = Float4( ( quad->range.sx + quad->range.ex ) / 2, ( quad->range.sy + quad->range.ey ) / 2, quad->range.ex, quad->range.ey );
			}
			quad->node = true;
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
				quad->node = false;
				dprintf( "WARNING: Sludge node generated!\n" );
			} else {
				quad->lines.clear();
			}
		}
		if( quad->node ) {
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
	}
};

void Collider::quadRemove( const Float4 &range, Collide *ptr, Quad *quad ) {
	if( rectrectintersect( range, quad->range ) ) {
		if( quad->node ) {
			int ct = 0;
			bool childnode = false;
			for( int i = 0; i < 4; i++ ) {
				quadRemove( range, ptr, &quad->quads[ i ] );
				ct += quad->quads[ i ].lines.size();
				if( quad->quads[ i ].node )
					childnode = true;
			}
			if( !childnode && ct < LEAF_COLLAPSE_THRESH ) {
				quad->node = false;
				for( int i = 0; i < 4; i++ ) {
					quad->lines.insert( quad->lines.end(), quad->quads[ i ].lines.begin(), quad->quads[ i ].lines.end() );
					quad->quads[ i ].lines.clear();
					quad->quads[ i ].sludge = false;	// yeah yeah.
				}
				sort( quad->lines.begin(), quad->lines.end() );
				quad->lines.erase( unique( quad->lines.begin(), quad->lines.end() ), quad->lines.end() );
			}
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
*/#endif
