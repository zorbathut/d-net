
#include "collide.h"

#include <algorithm>
#include <cmath>

using namespace std;

#include "debug.h"
#include "rng.h"
#include "util.h"
#include "args.h"
#include "gfx.h"

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
      Coord4 cline = *linepos + *linevel * tt;
      pos = Coord2(cline.sx, cline.sy) + Coord2(cline.ex - cline.sx, cline.ey - cline.sy) * u;
    }
  }
  return make_pair(cBc, pos);
}

inline int getIndexCount(int players) {
  return players * 2 + 1;
}
int getIndex( int players, int category, int gid ) {
  if(category == CGR_WALL) {
    CHECK(gid == 0);
    return 0;
  } else if(category == CGR_PLAYER) {
    CHECK(gid >= 0 && gid < players);
    return gid + 1;
  } else if(category == CGR_PROJECTILE) {
    CHECK(gid >= 0 && gid < players);
    return players + gid + 1;
  } else {
    return 0;
  }
}
pair< int, int > reverseIndex( int players, int index ) {
  if(index == 0)
    return make_pair(int(CGR_WALL), 0);
  else if(index < players + 1)
    return make_pair(int(CGR_PLAYER), index - 1);
  else if(index < players * 2 + 1)
    return make_pair(int(CGR_PROJECTILE), index - players - 1);
  else
    CHECK(0);
}

bool canCollidePlayer(int players, int indexa, int indexb, const vector<int> &teams) {
  pair<int, int> ar = reverseIndex(players, indexa);
  pair<int, int> br = reverseIndex(players, indexb);
  // Two things can't collide if they're part of the same ownership group (ignoring walls which are different)
  if(ar.second == br.second && ar.first != CGR_WALL && br.first != CGR_WALL)
    return false;
  // Nothing can collide with itself
  if(ar == br)
    return false;
  // Projectiles on the same team don't collide
  if(ar.first == CGR_PROJECTILE && br.first == CGR_PROJECTILE && teams[ar.second] == teams[br.second])
    return false;
  return true;
}
bool canCollideProjectile(int players, int indexa, int indexb, const vector<int> &teams) {
  pair<int, int> ar = reverseIndex(players, indexa);
  pair<int, int> br = reverseIndex(players, indexb);
  // Two things can't collide if they're part of the same ownership group (ignoring walls which are different)
  if(ar.second == br.second && ar.first != CGR_WALL && br.first != CGR_WALL)
    return false;
  // Nothing can collide with itself
  if(ar == br)
    return false;
  // Two things can't collide if neither of them are a projectile
  if(ar.first != CGR_PROJECTILE && br.first != CGR_PROJECTILE)
    return false;
  // Projectiles on the same team don't collide
  if(ar.first == CGR_PROJECTILE && br.first == CGR_PROJECTILE && teams[ar.second] == teams[br.second])
    return false;
  // That's pretty much all.
  return true;
}

void ColliderZone::addToken(int groupid, int token, const Coord4 &line, const Coord4 &direction) {
  int fd = 0;
  for(fd = 0; fd < lastItem; fd++)
    if(items[fd].first == groupid)
      break;
  if(fd == lastItem) {
    lastItem++;
    if(fd == items.size())
      items.push_back(make_pair(groupid, vector< pair< int, pair< Coord4, Coord4 > > >()));
    else
      items[fd].first = groupid;
  }
  items[fd].second.push_back(make_pair(token, make_pair(line, direction)));
}
void ColliderZone::clearToken(int groupid, int token) {
  for(int i = 0; i < lastItem; i++) {
    if(items[i].first == groupid) {
      int tid;
      for(tid = 0; tid < items[i].second.size(); tid++)
        if(items[i].second[tid].first == token)
          break;
      if(tid == items[i].second.size()) // Nothing!
        break;
      vector< pair< int, pair< Coord4, Coord4 > > > gnu(items[i].second.begin(), items[i].second.begin() + tid);
      for( ; tid < items[i].second.size(); tid++)
        if(items[i].second[tid].first != token)
          gnu.push_back(items[i].second[tid]);
      gnu.swap(items[i].second);
    }
  }
}

void ColliderZone::clearGroup(int groupid) {
  for(int fd = 0; fd < items.size(); fd++)
    if(items[fd].first == groupid)
      items[fd].second.clear();
}

bool ColliderZone::checkSimpleCollision(int groupid, const vector<Coord4> &line, const char *collidematrix) const {
  for(int x = 0; x < lastItem; x++) {
     if(!collidematrix[groupid * getIndexCount(players) + items[x].first])
      continue;
    const vector< pair< int, pair< Coord4, Coord4 > > > &tx = items[x].second;
    for( int xa = 0; xa < tx.size(); xa++ ) {
      for( int ya = 0; ya < line.size(); ya++ ) {
        if(linelineintersect(tx[xa].second.first, line[ya]))
          return true;
      }
    }
  }
  return false;
}

void ColliderZone::processSimple(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const {
  for( int x = 0; x < lastItem; x++ ) {
    for( int y = x + 1; y < lastItem; y++ ) {
      if(!collidematrix[items[x].first * getIndexCount(players) + items[y].first])
        continue;
      const vector< pair< int, pair< Coord4, Coord4 > > > &tx = items[x].second;
      const vector< pair< int, pair< Coord4, Coord4 > > > &ty = items[y].second;
      for( int xa = 0; xa < tx.size(); xa++ ) {
        for( int ya = 0; ya < ty.size(); ya++ ) {
          if(linelineintersect(tx[xa].second.first, ty[ya].second.first))
            clds->push_back(make_pair(0, CollideData(CollideId(reverseIndex(players, items[x].first), tx[xa].first), CollideId(reverseIndex(players, items[y].first), ty[ya].first), Coord2())));
        }
      }
    }
  }
}
void ColliderZone::processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const {
  for( int x = 0; x < lastItem; x++ ) {
    for( int y = x + 1; y < lastItem; y++ ) {
      if(!collidematrix[items[x].first * getIndexCount(players) + items[y].first])
        continue;
      const vector< pair< int, pair< Coord4, Coord4 > > > &tx = items[x].second;
      const vector< pair< int, pair< Coord4, Coord4 > > > &ty = items[y].second;
      for( int xa = 0; xa < tx.size(); xa++ ) {
        for( int ya = 0; ya < ty.size(); ya++ ) {
          pair<Coord, Coord2> tcol = getCollision( tx[ xa ].second.first, tx[ xa ].second.second, ty[ ya ].second.first, ty[ ya ].second.second );
          if( tcol.first == NOCOLLIDE )
          	continue;
          CHECK( tcol.first >= 0 && tcol.first <= 1 );
          clds->push_back(make_pair(tcol.first, CollideData(CollideId(reverseIndex(players, items[x].first), tx[xa].first), CollideId(reverseIndex(players, items[y].first), ty[ya].first), tcol.second)));
        }
      }
    }
  }
}

void ColliderZone::render(const Coord4 &bbox) const {
  Coord4 tbx = bbox;
  expandBoundBox(&tbx, Coord(0.8f));
  setColor(Color(1.0, 1.0, 1.0) * 0.2 * items.size());
  drawRect(tbx.toFloat(), 0.5);
}

void ColliderZone::reset(int wallid) {
  int wallchunk = -1;
  for(int i = 0; i < lastItem; i++) {
    if(items[i].first == wallid) {
      CHECK(wallchunk == -1);
      wallchunk = i;
    } else {
      items[i].second.clear();
    }
  }
  if(wallchunk != -1) {
    swap(items[wallchunk], items[0]);
    lastItem = 1;
  } else {
    lastItem = 0;
  }
}

void ColliderZone::full_reset() {
  lastItem = 0;
  for(int i = 0; i < items.size(); i++)
    items[i].second.clear();
}

ColliderZone::ColliderZone() {
  lastItem = 0;
};
ColliderZone::ColliderZone(int in_players) {
  players = in_players;
  lastItem = 0;
}

void Collider::resetNonwalls(int mode, const Coord4 &bounds, const vector<int> &teams) {
  CHECK(state == CSTA_UNINITTED || state == CSTA_WAIT);
  CHECK(teams.size() == players);
  
  Coord4 zbounds = snapToEnclosingGrid(bounds, MATRIX_RES);
  
  int nzxs = (zbounds.sx / MATRIX_RES).toInt();
  int nzys = (zbounds.sy / MATRIX_RES).toInt();
  int nzxe = (zbounds.ex / MATRIX_RES).toInt();
  int nzye = (zbounds.ey / MATRIX_RES).toInt();
  if(nzxs != zxs || nzys != zys || nzxe != zxe || nzye != zye) {
    CHECK(!full_reset);
    dprintf("Full collider reset!");
    full_reset = true;
    zxs = nzxs;
    zys = nzys;
    zxe = nzxe;
    zye = nzye;
  }
  
  int walltoken = getIndex(players, CGR_WALL, 0);
  
  zone.resize(zxe - zxs);
  for(int i = 0; i < zone.size(); i++) {
    for(int j = 0; j < zone[i].size(); j++) {
      if(full_reset) {
        zone[i][j].full_reset();
      } else {
        zone[i][j].reset(walltoken);
      }
    }
    zone[i].resize(zye - zys, ColliderZone(players));
  }
  
  collidematrix.clear();
  
  if(mode == COM_PLAYER) {
    for(int i = 0; i < players * 2 + 1; i++)
      for(int j = 0; j < players * 2 + 1; j++)
        collidematrix.push_back(canCollidePlayer(players, i, j, teams));
  } else if(mode == COM_PROJECTILE) {
    for(int i = 0; i < players * 2 + 1; i++)
      for(int j = 0; j < players * 2 + 1; j++)
        collidematrix.push_back(canCollideProjectile(players, i, j, teams));
  } else {
    CHECK(0);
  }
  
  /*
  for(int i = 0; i < players * 2 + 1; i++) {
    string tp;
    for(int j = 0; j < players * 2 + 1; j++)
      tp += collidematrix[j + i * ( players * 2 + 1 )] + '0';
    dprintf("%s\n", tp.c_str());
  }*/
  
  state = CSTA_WAIT;
}
bool Collider::consumeFullReset() {
  bool rv = full_reset;
  full_reset = false;
  return rv;
}

void Collider::startToken( int toki ) {
  CHECK( state == CSTA_ADD && curpush != -1 );
  curtoken = toki;
}
void Collider::token( const Coord4 &line, const Coord4 &direction ) {
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
    if(!(txs < zxe && tys < zye && txe > zxs && tye > zys)) {
      dprintf("%d, %d, %d, %d\n", txs, tys, txe, tye);
      dprintf("%d, %d, %d, %d\n", zxs, zys, zxe, zye);
      dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
      dprintf("%f, %f, %f, %f\n", line.sx.toFloat(), line.sy.toFloat(), line.ex.toFloat(), line.ey.toFloat());
      dprintf("%f, %f, %f, %f\n", direction.sx.toFloat(), direction.sy.toFloat(), direction.ex.toFloat(), direction.ey.toFloat());
      dprintf("-----\n");
      dprintf("%s, %s, %s, %s\n", area.sx.rawstr().c_str(), area.sy.rawstr().c_str(), area.ex.rawstr().c_str(), area.ey.rawstr().c_str());
      dprintf("%s, %s, %s, %s\n", line.sx.rawstr().c_str(), line.sy.rawstr().c_str(), line.ex.rawstr().c_str(), line.ey.rawstr().c_str());
      dprintf("%s, %s, %s, %s\n", direction.sx.rawstr().c_str(), direction.sy.rawstr().c_str(), direction.ex.rawstr().c_str(), direction.ey.rawstr().c_str());
      dprintf("-----\n");
      dprintf("Area bounds: %f,%f %f,%f\n", (zxs * MATRIX_RES).toFloat(), (zys * MATRIX_RES).toFloat(), (zxe * MATRIX_RES).toFloat(), (zye * MATRIX_RES).toFloat());
      CHECK(0);
    }
    for(int x = txs; x < txe; x++)
      for(int y = tys; y < tye; y++)
        zone[x - zxs][y - zys].addToken(curpush, curtoken, line, direction);
  } else {
    CHECK(0);
  }
}

void Collider::token(const Coord4 &line) {
  if( state == CSTA_ADD ) {
    CHECK( state == CSTA_ADD && curpush != -1 && curtoken != -1 );
    Coord4 area(min(line.sx, line.ex), min(line.sy, line.ey), max(line.sx, line.ex), max(line.sy, line.ey));
    area = snapToEnclosingGrid(area, MATRIX_RES);
    int txs = max((area.sx / MATRIX_RES).toInt(), zxs);
    int tys = max((area.sy / MATRIX_RES).toInt(), zys);
    int txe = min((area.ex / MATRIX_RES).toInt(), zxe);
    int tye = min((area.ey / MATRIX_RES).toInt(), zye);
    if(!(txs < zxe && tys < zye && txe > zxs && tye > zys)) {
      dprintf("%d, %d, %d, %d\n", txs, tys, txe, tye);
      dprintf("%d, %d, %d, %d\n", zxs, zys, zxe, zye);
      dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
      dprintf("%f, %f, %f, %f\n", line.sx.toFloat(), line.sy.toFloat(), line.ex.toFloat(), line.ey.toFloat());
      CHECK(0);
    }
    for(int x = txs; x < txe; x++)
      for(int y = tys; y < tye; y++)
        zone[x - zxs][y - zys].addToken(curpush, curtoken, line, Coord4(0, 0, 0, 0));
  } else {
    CHECK(0);
  }
}
void Collider::clearToken(int toki) {
  for(int i = 0; i < zone.size(); i++)
    for(int j = 0; j < zone[i].size(); j++)
      zone[i][j].clearToken(curpush, toki);
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

void Collider::clearGroup(int category, int gid) {
  int groupid = getIndex(players, category, gid);
  for(int i = 0; i < zone.size(); i++)
    for(int j = 0; j < zone[i].size(); j++)
      zone[i][j].clearGroup(groupid);
}

bool Collider::checkSimpleCollision(int category, int gid, const vector<Coord4> &line) const {
  if(line.size() == 0)
    return false;
  Coord4 area = startCBoundBox();
  for(int i = 0; i < line.size(); i++) {
    addToBoundBox(&area, line[i].sx, line[i].sy);
    addToBoundBox(&area, line[i].ex, line[i].ey);
  }
  Coord4 pa = area;
  area.sx -= MATRIX_RES / 4;
  area.sy -= MATRIX_RES / 4;
  area.ex += MATRIX_RES / 4;
  area.ey += MATRIX_RES / 4;
  area = snapToEnclosingGrid(area, MATRIX_RES);
  int txs = max((area.sx / MATRIX_RES).toInt(), zxs);
  int tys = max((area.sy / MATRIX_RES).toInt(), zys);
  int txe = min((area.ex / MATRIX_RES).toInt(), zxe);
  int tye = min((area.ey / MATRIX_RES).toInt(), zye);
  if(!(txs < zxe && tys < zye && txe > zxs && tye > zys)) {
    dprintf("%d, %d, %d, %d\n", txs, tys, txe, tye);
    dprintf("%d, %d, %d, %d\n", zxs, zys, zxe, zye);
    dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
    dprintf("%f, %f, %f, %f\n", pa.sx.toFloat(), pa.sy.toFloat(), pa.ex.toFloat(), pa.ey.toFloat());
    CHECK(0);
  }
  for(int x = txs; x < txe; x++)
    for(int y = tys; y < tye; y++)
      if(zone[x - zxs][y - zys].checkSimpleCollision(getIndex(players, category, gid), line, &*collidematrix.begin()))
        return true;
  return false;
}

void Collider::processSimple() {
  CHECK( state == CSTA_WAIT );
  state = CSTA_PROCESSED;
  collides.clear();
  curcollide = -1;
  
  vector<pair<Coord, CollideData> > clds;
  
  // TODO: Don't bother processing unique pairs more than once?
  for(int i = 0; i < zone.size(); i++)
    for(int j = 0; j < zone[i].size(); j++)
      zone[i][j].processSimple(&clds, &*collidematrix.begin());
  
  sort(clds.begin(), clds.end());
  clds.erase(unique(clds.begin(), clds.end()), clds.end());
}
void Collider::processMotion() {
  CHECK( state == CSTA_WAIT );
  state = CSTA_PROCESSED;
  collides.clear();
  curcollide = -1;
  
  vector<pair<Coord, CollideData> > clds;
  
  // TODO: Don't bother processing unique pairs more than once?
  for(int i = 0; i < zone.size(); i++)
    for(int j = 0; j < zone[i].size(); j++)
      zone[i][j].processMotion(&clds, &*collidematrix.begin());
  
  sort(clds.begin(), clds.end());
  clds.erase(unique(clds.begin(), clds.end()), clds.end());
  
  {
    set<CollideId> hit;
    for(int i = 0; i < clds.size(); i++) {
      if(clds[i].second.lhs.category == CGR_PROJECTILE && hit.count(clds[i].second.lhs))
        continue;
      if(clds[i].second.rhs.category == CGR_PROJECTILE && hit.count(clds[i].second.rhs))
        continue;
      if(clds[i].second.lhs.category == CGR_PROJECTILE)
        hit.insert(clds[i].second.lhs);
      if(clds[i].second.rhs.category == CGR_PROJECTILE)
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

void Collider::finishProcess() {
  CHECK(state == CSTA_PROCESSED);
  state = CSTA_WAIT;
}

Collider::Collider() { state = -30; curpush = -1; curtoken = -1; log = false; full_reset = false; };
Collider::Collider(int playercount) { state = CSTA_UNINITTED; curpush = -1; curtoken = -1; log = false; full_reset = false; players = playercount; };
Collider::~Collider() { };

DECLARE_bool(debugGraphics);

void Collider::render() const {
  if(FLAGS_debugGraphics) {
    for(int i = 0; i < zone.size(); i++)
      for(int j = 0; j < zone[i].size(); j++)
        zone[i][j].render(Coord4((zxs + i) * MATRIX_RES, (zys + j) * MATRIX_RES, (zxs + i + 1) * MATRIX_RES, (zys + j + 1) * MATRIX_RES));
  }
};

