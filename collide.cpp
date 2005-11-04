
#include "collide.h"

#include <algorithm>
#include <cmath>

using namespace std;

#include "debug.h"
#include "rng.h"
#include "util.h"
#include "args.h"

DECLARE_bool(verboseCollisions);

const Coord NOCOLLIDE = Coord(-1000000000);
const Coord MATRIX_RES = 20;

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
	if( abs(a2) < Coord(0.00001f) ) {
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
            if(!(abs( a * rv.first * rv.first + b * rv.first + c ) < 1000000))
                dprintf( "debugtest: %f resolves to %f (%f, %f, %f, %f)\n", rv.first.toFloat(), (a * rv.first * rv.first + b * rv.first + c).toFloat(), a.toFloat(), b.toFloat(), c.toFloat(), rv.first.toFloat() );
			CHECK( abs( a * rv.first * rv.first + b * rv.first + c ) < 1000000 );
		}
		if( rv.second != NOCOLLIDE ) {
			if(!(abs( a * rv.second * rv.second + b * rv.second + c ) < 1000000))
                dprintf( "debugtest: %f resolves to %f (%f, %f, %f, %f)\n", rv.second.toFloat(), (a * rv.second * rv.second + b * rv.second + c).toFloat(), a.toFloat(), b.toFloat(), c.toFloat(), rv.second.toFloat() );
            CHECK( abs( a * rv.second * rv.second + b * rv.second + c ) < 1000000 );
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

pair<Coord, Coord2> getCollision( const Coord4 &l1p, const Coord4 &l1v, const Coord4 &l2p, const Coord4 &l2v ) {
	Coord cBc = NOCOLLIDE;
    Coord2 pos;
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
        Coord2 tpos;
		for( int j = 0; j < 2; j++ ) {
			Coord tt;
			if( j ) {
				tt = tbv.second;
			} else {
				tt = tbv.first;
			}
			//if( verbosified && tt != NOCOLLIDE )
				//dprintf( "%d, %d is %f\n", i, j, tt );
			if( tt < 0 || tt > 1 || ( cBc != NOCOLLIDE && tt > cBc ) )
				continue;
			Coord u = getu( *linepos, *linevel, *ptposvel, tt );
			if( u < 0 || u > 1 )
				continue;
			cBc = tt;
            Coord4 cline = lerp(*linepos, *linevel, tt);
            pos = lerp(Coord2(cline.sx, cline.sy), Coord2(cline.ex - cline.sx, cline.ey - cline.sy), u);
		}
	}
	return make_pair(cBc, pos);
}

int getIndex( int players, int category, int gid ) {
    if( category == -1 ) {
        CHECK( gid == 0 );
        return 0;
    } else {
        CHECK( category == 0 || category == 1 );
        CHECK( gid >= 0 && gid < players );
        return players * category + gid + 1;
    }
}
pair< int, int > reverseIndex( int players, int index ) {
    if( index == 0 )
        return make_pair( -1, 0 );
    else {
        pair< int, int > out( ( index - 1 ) / players, ( index - 1 ) % players );
        CHECK( out.first == 0 || out.first == 1 );
        return out;
    }
}
bool canCollide( int players, int indexa, int indexb ) {
    pair<int, int> ar = reverseIndex(players, indexa);
    pair<int, int> br = reverseIndex(players, indexb);
    // Two things can't collide if they're part of the same ownership group
    if(ar.second == br.second && ar.first != -1 && br.first != -1)
        return false;
    // Two things can't collide if neither of them are a projectile
    if(ar.first != 1 && br.first != 1)
        return false;
    // That's pretty much all.
    return true;
}

void ColliderZone::addToken(int groupid, int token, const Coord4 &line, const Coord4 &direction) {
    items[groupid].push_back(make_pair(token, make_pair(line, direction)));
}
void ColliderZone::process(vector<pair<Coord, CollideData> > *clds) const {
	for( int x = 0; x < items.size(); x++ ) {
		for( int y = x + 1; y < items.size(); y++ ) {
            if(!canCollide( players, x, y))
                continue;
			for( int xa = 0; xa < items[ x ].size(); xa++ ) {
				for( int ya = 0; ya < items[ y ].size(); ya++ ) {
					pair<Coord, Coord2> tcol = getCollision( items[ x ][ xa ].second.first, items[ x ][ xa ].second.second, items[ y ][ ya ].second.first, items[ y ][ ya ].second.second );
					if( tcol.first == NOCOLLIDE )
						continue;
					CHECK( tcol.first >= 0 && tcol.first <= 1 );
                    clds->push_back(make_pair(tcol.first, CollideData(CollideId(reverseIndex(players, x), items[x][xa].first), CollideId(reverseIndex(players, y), items[y][ya].first), tcol.second)));
				}
			}
		}
	}
}

ColliderZone::ColliderZone() { };
ColliderZone::ColliderZone(int in_players) {
    items.resize(in_players * 2 + 1);
    players = in_players;
}

void Collider::reset(int in_players, const Coord4 &bounds) {
	CHECK( state == CSTA_WAIT || state == CSTA_PROCESSED );
	players = in_players;
    
    Coord4 zbounds = snapToEnclosingGrid(bounds, MATRIX_RES);
    
    zxs = (zbounds.sx / MATRIX_RES).toInt();
    zys = (zbounds.sy / MATRIX_RES).toInt();
    zxe = (zbounds.ex / MATRIX_RES).toInt();
    zye = (zbounds.ey / MATRIX_RES).toInt();
    zone.clear();
    zone.resize(zxe - zxs, vector<ColliderZone>(zye - zys, ColliderZone( players )));
    
    state = CSTA_WAIT;
}

void Collider::startToken( int toki ) {
	CHECK( state == CSTA_ADD && curpush != -1 );
    curtoken = toki;
}
void Collider::token( const Coord4 &line, const Coord4 &direction ) {
    if(log) {
        dprintf("Collide in: %f,%f-%f,%f delta %f,%f-%f,%f\n",
            line.sx.toFloat(), line.sy.toFloat(), line.ex.toFloat(), line.ey.toFloat(),
            direction.sx.toFloat(), direction.sy.toFloat(), direction.ex.toFloat(), direction.ey.toFloat());
    }
    if( state == CSTA_ADD ) {
        CHECK( state == CSTA_ADD && curpush != -1 && curtoken != -1 );
        Coord4 area = startCBoundBox();
        addToBoundBox(&area, line.sx, line.sy);
        addToBoundBox(&area, line.ex, line.ey);
        addToBoundBox(&area, line.sx + direction.sx, line.sy + direction.sy);
        addToBoundBox(&area, line.ex + direction.ex, line.ey + direction.ey);
        area = snapToEnclosingGrid(area, MATRIX_RES);
        int txs = max((area.sx / MATRIX_RES).toInt(), zxs);
        int tys = max((area.sy / MATRIX_RES).toInt(), zys);
        int txe = min((area.ex / MATRIX_RES).toInt(), zxe);
        int tye = min((area.ey / MATRIX_RES).toInt(), zye);
        CHECK(txs < zxe && tys < zye && txe > zxs && tye > zys);
        for(int x = txs; x < txe; x++)
            for(int y = tys; y < tye; y++)
                zone[x - zxs][y - zys].addToken(curpush, curtoken, line, direction);
    } else {
        CHECK(0);
    }
}

void Collider::addThingsToGroup( int category, int gid, bool ilog ) {
    CHECK( state == CSTA_WAIT && curpush == -1 && curtoken == -1 );
    state = CSTA_ADD;
    log = ilog;
    curpush = getIndex(players, category, gid);
}
void Collider::endAddThingsToGroup() {
    CHECK( state == CSTA_ADD && curpush != -1 );
    state = CSTA_WAIT;
    log = false;
    curpush = -1;
    curtoken = -1;
}

void Collider::process() {
	CHECK( state == CSTA_WAIT );
    state = CSTA_PROCESSED;
    collides.clear();
    curcollide = -1;
    
    vector<pair<Coord, CollideData> > clds;
    
    // TODO: Don't bother processing unique pairs more than once?
    for(int i = 0; i < zone.size(); i++)
        for(int j = 0; j < zone[i].size(); j++)
            zone[i][j].process(&clds);
	
    sort(clds.begin(), clds.end());
    clds.erase(unique(clds.begin(), clds.end()), clds.end());
    
    {
        set<CollideId> hit;
        for(int i = 0; i < clds.size(); i++) {
            if(clds[i].second.lhs.category == 1 && hit.count(clds[i].second.lhs))
                continue;
            if(clds[i].second.rhs.category == 1 && hit.count(clds[i].second.rhs))
                continue;
            if(clds[i].second.lhs.category == 1)
                hit.insert(clds[i].second.lhs);
            if(clds[i].second.rhs.category == 1)
                hit.insert(clds[i].second.rhs);
            collides.push_back(clds[i].second);
        }
    }
}

bool Collider::next() {
    curcollide++;
    return curcollide < collides.size();
}

const CollideData &Collider::getData() const {
    CHECK(state == CSTA_PROCESSED);
    CHECK(curcollide >= 0 && curcollide < collides.size());
	return collides[curcollide];
}

Collider::Collider() { state = 0; curpush = -1; curtoken = -1; log = false; };
Collider::~Collider() { };

void Collider::render() const { };

