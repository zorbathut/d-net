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
			CHECK( fabs( a * rv.first * rv.first + b * rv.first + c ) < 1e6 );
			//dprintf( "debugtest: %f resolves to %f\n", rv.first, a * rv.first * rv.first + b * rv.first + c );
		}
		if( rv.second != NOCOLLIDE ) {
			CHECK( fabs( a * rv.second * rv.second + b * rv.second + c ) < 1e6 );
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
				CHECK( 0 );
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

void Collider::reset( int in_players ) {
	CHECK( state == 0 );
	players = in_players;
    items.resize( in_players * 2 + 1 );
    for( int i = 0; i < items.size(); i++ )
        items[ i ].clear();
	ctime = 0;
}

void Collider::startToken( int toki ) {
	CHECK( ( state == 1 || state == 2 ) && curpush != -1 );
    curtoken = toki;
}
void Collider::token( const Float4 &line, const Float4 &direction ) {
    if( state == 1 ) {
        CHECK( state == 1 && curpush != -1 && curtoken != -1 );
        CHECK( ctime == 0 || ( direction.sx == 0 && direction.sy == 0 && direction.ex == 0 && direction.ey == 0 ) );
        items[ curpush ].push_back( make_pair( curtoken, make_pair( line, direction ) ) );
    } else if( state == 2 ) {
        CHECK( state == 2 && curpush != -1 && curtoken != -1 );
        vector< pair< int, pair< Float4, Float4 > > >::iterator jit = find( items[ curpush ].begin(), items[ curpush ].end(), make_pair( curtoken, make_pair( line, direction ) ) );
        CHECK( jit != items[ curpush ].end() );
        items[ curpush ].erase( jit );
    }
}

void Collider::clearGroup( int category, int gid ) {
    CHECK( state == 0 && curpush == -1 && curtoken == -1 );
    items[ getIndex( category, gid ) ].clear();
}

void Collider::addThingsToGroup( int category, int gid ) {
    CHECK( state == 0 && curpush == -1 && curtoken == -1 );
    state = 1;
    curpush = getIndex( category, gid );
}
void Collider::endAddThingsToGroup() {
    CHECK( state == 1 && curpush != -1 );
    state = 0;
    curpush = -1;
    curtoken = -1;
}

void Collider::removeThingsFromGroup( int category, int gid ) {
    CHECK( state == 0 && curpush == -1 && curtoken == -1 );
    state = 2;
    curpush = getIndex( category, gid );
}
void Collider::endRemoveThingsFromGroup() {
    CHECK( state == 2 && curpush != -1 );
    state = 0;
    curpush = -1;
    curtoken = -1;
}

bool Collider::doProcess() {
	CHECK( state == 0 );
    
    for(set<int>::iterator itr = moved.begin(); itr != moved.end(); itr = moved.begin()) {
        for( int alter = 0; alter < items.size(); alter++ ) {
            if( !canCollide( *itr, alter ) )
                continue;
            for( int x = 0; x < items[ *itr ].size(); x++ ) {
                for( int y = 0; y < items[ alter ].size(); y++ ) {
                    if( linelineintersectend( lerp( items[ alter ][ y ].second.first, items[ alter ][ y ].second.second, ctime ), lerp( items[ *itr ][ x ].second.first, items[ *itr ][ x ].second.second, ctime ) ) ) {
                        CHECK( reverseIndex( alter ).first == 1 );
                        lhs.first = reverseIndex( *itr );
                        lhs.second = items[ *itr ][ x ].first;
                        rhs.first = reverseIndex( alter );
                        rhs.second = items[ alter ][ y ].first;
                        CHECK( rhs.first.first == 1 );
                        return true;
                    }
                }
            }
        }
        moved.erase( itr );
    }
    
	float firstintersect = 2.0;
    int lhsx = 0;
    int rhsx = 0;
	for( int x = 0; x < items.size(); x++ ) {
		for( int y = x + 1; y < items.size(); y++ ) {
            if( !canCollide( x, y ) )
                continue;
			for( int xa = 0; xa < items[ x ].size(); xa++ ) {
				for( int ya = 0; ya < items[ y ].size(); ya++ ) {
					float tcol = getCollision( items[ x ][ xa ].second.first, items[ x ][ xa ].second.second, items[ y ][ ya ].second.first, items[ y ][ ya ].second.second, ctime );
					if( tcol == NOCOLLIDE )
						continue;
					CHECK( tcol >= ctime  && tcol >= 0 && tcol <= 1 );
					if( tcol < firstintersect ) {
						firstintersect = tcol;
						lhsx = x;
						lhs.second = items[ x ][ xa ].first;
						rhsx = y;
						rhs.second = items[ y ][ ya ].first;
					}
				}
			}
		}
	}
	ctime = firstintersect;
    lhs.first = reverseIndex( lhsx );
    rhs.first = reverseIndex( rhsx );
	return firstintersect < 2.0;
}

float Collider::getCurrentTimestamp() const {
	CHECK( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return ctime;
}
void Collider::setCurrentTimestamp( float ntime ) {
	CHECK( state == 0 && ntime <= 1.0 && ntime >= 0.0 );
	ctime = ntime;
}

pair< pair< int, int >, int > Collider::getLhs() const {
	CHECK( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return lhs;
}
pair< pair< int, int >, int > Collider::getRhs() const {
	CHECK( state == 0 && ctime <= 1.0 && ctime >= 0.0 );
	return rhs;
}

void Collider::flagAsMoved( int category, int gid ) {
    moved.insert( getIndex( category, gid ) );
}

bool Collider::testCollideSingle( int lhs, int rhs, bool print ) const {
    CHECK( lhs >= 0 && lhs < items.size() );
    CHECK( rhs >= 0 && rhs < items.size() );
    for( int y = 0; y < items[ lhs ].size(); y++ ) {
        for( int k = 0; k < items[ rhs ].size(); k++ ) {
            if( linelineintersectend( lerp( items[ lhs ][ y ].second.first, items[ lhs ][ y ].second.second, ctime ), lerp( items[ rhs ][ k ].second.first, items[ rhs ][ k ].second.second, ctime ) ) ) {
                if(print) {
                    Float4 lhl = lerp( items[ lhs ][ y ].second.first, items[ lhs ][ y ].second.second, ctime );
                    Float4 rhl = lerp( items[ rhs ][ k ].second.first, items[ rhs ][ k ].second.second, ctime );
                    dprintf("%d/%d with %d/%d\n", lhs, y, rhs, k);
                    dprintf("%f,%f - %f,%f vs %f,%f - %f,%f\n", lhl.sx, lhl.sy, lhl.ex, lhl.ey, rhl.sx, rhl.sy, rhl.ex, rhl.ey);
                }
                return true;
            }
        }
    }
    return false;
}
bool Collider::testCollideAgainst( int active ) const {
    int actindex;
    if( active == -1 )
        actindex = getIndex( -1, 0 );
      else
        actindex = getIndex( 0, active );
    for( int i = -1; i < players; i++ ) {
        if( active == i )
            continue;
        if( i == -1 ) {
            if( testCollideSingle( getIndex( -1, 0 ), actindex ) )
                return true;
        } else {
            if( testCollideSingle( getIndex( 0, i ), actindex ) )
                return true;
        }
    }
    return false;
}
bool Collider::testCollideAll(bool print) const {
    for( int i = -1; i < players; i++ ) {
        for( int j = i + 1; j < players; j++ ) {
            if( i == -1 ) {
                if( testCollideSingle( getIndex( -1, 0 ), getIndex( 0, j ), print ) )
                    return true;
            } else {
                if( testCollideSingle( getIndex( 0, i ), getIndex( 0, j ), print ) )
                    return true;
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

bool Collider::canCollide( int indexa, int indexb ) const {
    if( indexa > indexb )
        swap( indexa, indexb );
    return !( indexb - indexa == players && indexa != 0 ) && indexa != indexb;
}
int Collider::getIndex( int category, int gid ) const {
    if( category == -1 ) {
        CHECK( gid == 0 );
        return 0;
    } else {
        CHECK( category == 0 || category == 1 );
        CHECK( gid >= 0 && gid < players );
        return players * category + gid + 1;
    }
}
pair< int, int > Collider::reverseIndex( int index ) const {
    if( index == 0 )
        return make_pair( -1, 0 );
    else {
        pair< int, int > out( ( index - 1 ) / players, ( index - 1 ) % players );
        CHECK( out.first == 0 || out.first == 1 );
        return out;
    }
}

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
				CHECK( 0 );
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


void Collider::startGroup() {
	newGroup.clear();
};

int Collider::endGroup() {

	int groupid;
	if( cleared.size() ) {
		groupid = cleared[ cleared.size() - 1 ];
		//dprintf( "Reclaimed group %d, %d\n", groupid, collides[ groupid ]->size() );
		CHECK( active[ groupid ] == false );
		CHECK( collides[ groupid ]->size() == 0 );
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
	CHECK( group >= 0 && group < active.size() );
	active[ group ] = false;
};
void Collider::enableGroup( int group ) {
	CHECK( group >= 0 && group < active.size() );
	active[ group ] = true;
};
void Collider::deleteGroup( int group ) {
	CHECK( group >= 0 && group < active.size() );
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
						CHECK( pts );
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
			CHECK( itr != quad->lines.end() );
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
