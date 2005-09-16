
#include "collide.h"

#include <algorithm>
#include <cmath>

using namespace std;

#include "debug.h"
#include "rng.h"
#include "util.h"

const Coord NOCOLLIDE = Coord(-1000000000);

pair< Coord, Coord > getLineCollision( const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel ) {
	Coord x1d = linepos.sx;
	Coord y1d = linepos.sy;
	Coord x2d = linepos.ex;
	Coord y2d = linepos.ey;
	Coord x1v = linevel.sx;
	Coord y1v = linevel.sy;
	Coord x2v = linevel.ex;
	Coord y2v = linevel.ey;
	Coord x3d = ptposvel.sx;
	Coord y3d = ptposvel.sy;
	Coord x3v = ptposvel.ex;
	Coord y3v = ptposvel.ey;
	Coord a = -x3v*y1v-x1v*y2v+x3v*y2v+x2v*y1v+x1v*y3v-x2v*y3v;
	Coord b = x2v*y1d-x3v*y1d+x2d*y1v-x3d*y1v-x1v*y2d+x3v*y2d-x1d*y2v+x3d*y2v+x1v*y3d-x2v*y3d+x1d*y3v-x2d*y3v;
	Coord c = x2d*y1d-x3d*y1d-x1d*y2d+x3d*y2d+x1d*y3d-x2d*y3d;
	//dprintf( "a is %f, b is %f, c is %f\n", a, b, c );
	Coord sqrii = b * b - 4 * a * c;
	//dprintf( "sqrii is %f\n", sqrii );
	Coord a2 = 2 * a;
	//dprintf( "a2 is %f\n", a2 );
	if( sqrii < 0 )
		return make_pair( NOCOLLIDE, NOCOLLIDE );
	pair< Coord, Coord > rv;
	if( a2 == 0 ) {
		if( b == 0 ) {
			return make_pair( NOCOLLIDE, NOCOLLIDE );
		}
		rv.first = -c / b;
		rv.second = NOCOLLIDE;
	} else {
		Coord sqrit = sqrt( sqrii );
		rv.first = ( -b + sqrit ) / a2;
		rv.second = ( -b - sqrit ) / a2;
	}
	{
		if( rv.first != NOCOLLIDE ) {
			CHECK( abs( a * rv.first * rv.first + b * rv.first + c ) < 1000000 );
			//dprintf( "debugtest: %f resolves to %f\n", rv.first, a * rv.first * rv.first + b * rv.first + c );
		}
		if( rv.second != NOCOLLIDE ) {
			CHECK( abs( a * rv.second * rv.second + b * rv.second + c ) < 1000000 );
			//dprintf( "debugtest: %f resolves to %f\n", rv.second, a * rv.second * rv.second + b * rv.second + c );
		}
	}
	return rv;
}

Coord sqr( Coord x ) {
	return x * x;
}

Coord getu( const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel, Coord t ) {
	Coord x1 = linepos.sx + linevel.sx * t;
	Coord y1 = linepos.sy + linevel.sy * t;
	Coord x2 = linepos.ex + linevel.ex * t;
	Coord y2 = linepos.ey + linevel.ey * t;
	Coord x3 = ptposvel.sx + ptposvel.ex * t;
	Coord y3 = ptposvel.sy + ptposvel.ey * t;
	return ( ( x3 - x1 ) * ( x2 - x1 ) + ( y3 - y1 ) * ( y2 - y1 ) ) / ( sqr( x2 - x1 ) + sqr( y2 - y1 ) );
}

Coord getCollision( const Coord4 &l1p, const Coord4 &l1v, const Coord4 &l2p, const Coord4 &l2v, Coord curtime ) {
	Coord cBc = NOCOLLIDE;
	Coord4 temp;
	for( int i = 0; i < 4; i++ ) {
		const Coord4 *linepos;
		const Coord4 *linevel;
		const Coord4 *ptposvel;
		switch( i ) {
			case 0:
				linepos = &l1p;
				linevel = &l1v;
				temp = Coord4( l2p.sx, l2p.sy, l2v.sx, l2v.sy );
				ptposvel = &temp;
				break;
			case 1:
				linepos = &l1p;
				linevel = &l1v;
				temp = Coord4( l2p.ex, l2p.ey, l2v.ex, l2v.ey );
				ptposvel = &temp;
				break;
			case 2:
				linepos = &l2p;
				linevel = &l2v;
				temp = Coord4( l1p.sx, l1p.sy, l1v.sx, l1v.sy );
				ptposvel = &temp;
				break;
			case 3:
				linepos = &l2p;
				linevel = &l2v;
				temp = Coord4( l1p.ex, l1p.ey, l1v.ex, l1v.ey );
				ptposvel = &temp;
				break;
			default:
				CHECK( 0 );
		}
        pair< Coord, Coord > tbv = getLineCollision( *linepos, *linevel, *ptposvel );
		for( int j = 0; j < 2; j++ ) {
			Coord tt;
			if( j ) {
				tt = tbv.second;
			} else {
				tt = tbv.first;
			}
			//if( verbosified && tt != NOCOLLIDE )
				//dprintf( "%d, %d is %f\n", i, j, tt );
			if( tt < 0 || tt > 1 || ( cBc != NOCOLLIDE && tt > cBc ) || tt < curtime )
				continue;
			Coord u = getu( *linepos, *linevel, *ptposvel, tt );
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
void Collider::token( const Coord4 &line, const Coord4 &direction ) {
    if( state == 1 ) {
        CHECK( state == 1 && curpush != -1 && curtoken != -1 );
        CHECK( ctime == 0 || ( direction.sx == 0 && direction.sy == 0 && direction.ex == 0 && direction.ey == 0 ) );
        items[ curpush ].push_back( make_pair( curtoken, make_pair( line, direction ) ) );
    } else if( state == 2 ) {
        CHECK( state == 2 && curpush != -1 && curtoken != -1 );
        vector< pair< int, pair< Coord4, Coord4 > > >::iterator jit = find( items[ curpush ].begin(), items[ curpush ].end(), make_pair( curtoken, make_pair( line, direction ) ) );
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
//                    if( linelineintersectend( lerp( items[ alter ][ y ].second.first, items[ alter ][ y ].second.second, ctime ), lerp( items[ *itr ][ x ].second.first, items[ *itr ][ x ].second.second, ctime ) ) ) {
                        CHECK( reverseIndex( alter ).first == 1 );
                        lhs.first = reverseIndex( *itr );
                        lhs.second = items[ *itr ][ x ].first;
                        rhs.first = reverseIndex( alter );
                        rhs.second = items[ alter ][ y ].first;
                        CHECK( rhs.first.first == 1 );
                        return true;
                    //}
                }
            }
        }
        moved.erase( itr );
    }
    
	Coord firstintersect = 2;
    int lhsx = 0;
    int rhsx = 0;
	for( int x = 0; x < items.size(); x++ ) {
		for( int y = x + 1; y < items.size(); y++ ) {
            if( !canCollide( x, y ) )
                continue;
			for( int xa = 0; xa < items[ x ].size(); xa++ ) {
				for( int ya = 0; ya < items[ y ].size(); ya++ ) {
					Coord tcol = getCollision( items[ x ][ xa ].second.first, items[ x ][ xa ].second.second, items[ y ][ ya ].second.first, items[ y ][ ya ].second.second, ctime );
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
	return firstintersect < 2;
}

Coord Collider::getCurrentTimestamp() const {
	CHECK( state == 0 && ctime <= 1 && ctime >= 0 );
	return ctime;
}
void Collider::setCurrentTimestamp( Coord ntime ) {
	CHECK( state == 0 && ntime <= 1 && ntime >= 0 );
	ctime = ntime;
}

pair< pair< int, int >, int > Collider::getLhs() const {
	CHECK( state == 0 && ctime <= 1 && ctime >= 0 );
	return lhs;
}
pair< pair< int, int >, int > Collider::getRhs() const {
	CHECK( state == 0 && ctime <= 1 && ctime >= 0 );
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
//            if( linelineintersectend( lerp( items[ lhs ][ y ].second.first, items[ lhs ][ y ].second.second, ctime ), lerp( items[ rhs ][ k ].second.first, items[ rhs ][ k ].second.second, ctime ) ) ) {
                if(print) {
                    Coord4 lhl = lerp( items[ lhs ][ y ].second.first, items[ lhs ][ y ].second.second, ctime );
                    Coord4 rhl = lerp( items[ rhs ][ k ].second.first, items[ rhs ][ k ].second.second, ctime );
                    dprintf("%d/%d with %d/%d\n", lhs, y, rhs, k);
                    dprintf("%f,%f - %f,%f vs %f,%f - %f,%f\n", lhl.sx.toFloat(), lhl.sy.toFloat(), lhl.ex.toFloat(), lhl.ey.toFloat(), rhl.sx.toFloat(), rhl.sy.toFloat(), rhl.ex.toFloat(), rhl.ey.toFloat());
                }
                return true;
 //           }
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
