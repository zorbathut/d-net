
#include "collide.h"

#include "args.h"
#include "gfx.h"
#include "util.h"
#include "adler32_util.h"

using namespace std;

DECLARE_bool(verboseCollisions);

const Coord NOCOLLIDE = Coord(-1000000000);

inline void norm(Coord *ref, const Coord &max) {
  if(*ref < 0 || *ref > 1 || *ref > max)
    *ref = NOCOLLIDE;
}

pair<Coord, Coord> getLineCollision(const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel) {
  /*Coord x1d = linepos.sx;
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
  Coord c = x2d*y1d-x3d*y1d-x1d*y2d+x3d*y2d+x1d*y3d-x2d*y3d;*/
  /*Coord a = -ptposvel.ex*linevel.sy-linevel.sx*linevel.ey+ptposvel.ex*linevel.ey+linevel.ex*linevel.sy+linevel.sx*ptposvel.ey-linevel.ex*ptposvel.ey;
  Coord b = linevel.ex*linepos.sy-ptposvel.ex*linepos.sy+linepos.ex*linevel.sy-ptposvel.sx*linevel.sy-linevel.sx*linepos.ey+ptposvel.ex*linepos.ey-linepos.sx*linevel.ey+ptposvel.sx*linevel.ey+linevel.sx*ptposvel.sy-linevel.ex*ptposvel.sy+linepos.sx*ptposvel.ey-linepos.ex*ptposvel.ey;
  Coord c = linepos.ex*linepos.sy-ptposvel.sx*linepos.sy-linepos.sx*linepos.ey+ptposvel.sx*linepos.ey+linepos.sx*ptposvel.sy-linepos.ex*ptposvel.sy;*/
  Coord a = ptposvel.ex*linevel.ey;
  a -= ptposvel.ex*linevel.sy;
  a -= linevel.sx*linevel.ey;
  a += linevel.ex*linevel.sy;
  a += linevel.sx*ptposvel.ey;
  a -= linevel.ex*ptposvel.ey;
  Coord b = linevel.ex*linepos.sy;
  b -= ptposvel.ex*linepos.sy;
  b += linepos.ex*linevel.sy;
  b -= ptposvel.sx*linevel.sy;
  b -= linevel.sx*linepos.ey;
  b += ptposvel.ex*linepos.ey;
  b -= linepos.sx*linevel.ey;
  b += ptposvel.sx*linevel.ey;
  b += linevel.sx*ptposvel.sy;
  b -= linevel.ex*ptposvel.sy;
  b += linepos.sx*ptposvel.ey;
  b -= linepos.ex*ptposvel.ey;
  Coord c = linepos.ex*linepos.sy;
  c -= ptposvel.sx*linepos.sy;
  c -= linepos.sx*linepos.ey;
  c += ptposvel.sx*linepos.ey;
  c += linepos.sx*ptposvel.sy;
  c -= linepos.ex*ptposvel.sy;
  //dprintf("a is %f, b is %f, c is %f\n", a, b, c);
  Coord sqrii = b * b;
  sqrii -= 4 * a * c;
  //dprintf("sqrii is %f\n", sqrii);
  Coord a2 = 2 * a;
  //dprintf("a2 is %f\n", a2);
  if(sqrii < 0)
    return make_pair(NOCOLLIDE, NOCOLLIDE);
  pair<Coord, Coord> rv;
  if(abs(a2) < Coord(0.00001f)) {
    if(b == 0) {
      return make_pair(NOCOLLIDE, NOCOLLIDE);
    }
    rv.first = -c / b;
    rv.second = NOCOLLIDE;
  } else {
    Coord sqrit = sqrt(sqrii);
    rv.first = (-b + sqrit) / a2;
    rv.second = (-b - sqrit) / a2;
  }
  {
    /*
    if(rv.first != NOCOLLIDE) {
      if(!(abs(a * rv.first * rv.first + b * rv.first + c) < 1000000))
        dprintf("debugtest: %f resolves to %f (%f, %f, %f, %f)\n", rv.first.toFloat(), (a * rv.first * rv.first + b * rv.first + c).toFloat(), a.toFloat(), b.toFloat(), c.toFloat(), rv.first.toFloat());
      CHECK(abs(a * rv.first * rv.first + b * rv.first + c) < 1000000);
    }
    if(rv.second != NOCOLLIDE) {
      if(!(abs(a * rv.second * rv.second + b * rv.second + c) < 1000000))
        dprintf("debugtest: %f resolves to %f (%f, %f, %f, %f)\n", rv.second.toFloat(), (a * rv.second * rv.second + b * rv.second + c).toFloat(), a.toFloat(), b.toFloat(), c.toFloat(), rv.second.toFloat());
      CHECK(abs(a * rv.second * rv.second + b * rv.second + c) < 1000000);
    }
    */
  }
  return rv;
}

Coord sqr(Coord x) {
  return x * x;
}

Coord getu(const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel, Coord t) {
  Coord x1 = linepos.sx + linevel.sx * t;
  Coord y1 = linepos.sy + linevel.sy * t;
  Coord x2 = linepos.ex + linevel.ex * t;
  Coord y2 = linepos.ey + linevel.ey * t;
  Coord x3 = ptposvel.sx + ptposvel.ex * t;
  Coord y3 = ptposvel.sy + ptposvel.ey * t;
  return ((x3 - x1) * (x2 - x1) + (y3 - y1) * (y2 - y1)) / (sqr(x2 - x1) + sqr(y2 - y1));
}

inline void proc2(Coord *cBc, Coord2 *pos, pair<Coord, Coord> *tbv, const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel) {
  norm(&tbv->first, *cBc);
  norm(&tbv->second, *cBc);
  if(likely(tbv->first == NOCOLLIDE)) {
    if(likely(tbv->second == NOCOLLIDE)) {
      return;
    } else {
      swap(tbv->first, tbv->second);
    }
  } else {
    if(likely(tbv->second == NOCOLLIDE)) {
    } else {
      if(tbv->first > tbv->second)
        swap(tbv->first, tbv->second);
    }
  }
  
  Coord u = getu(linepos, linevel, ptposvel, tbv->first);
  if(likely(u < 0 || u > 1)) {
    if(likely(tbv->second == NOCOLLIDE))
      return;
    u = getu(linepos, linevel, ptposvel, tbv->second);
    if(likely(u < 0 || u > 1))
      return;
    *cBc = tbv->second;
  } else {
    *cBc = tbv->first;
  }
  Coord4 cline = linepos + linevel * *cBc;
  *pos = Coord2(cline.sx, cline.sy) + Coord2(cline.ex - cline.sx, cline.ey - cline.sy) * u;
}

inline pair<Coord, Coord2> doCollisionNN(const Coord4 &l1p, const Coord4 &l1v, const Coord4 &l2p, const Coord4 &l2v) {
  /*if(!boxBoxIntersect(
      Coord4(
        min(min(l1p.sx, l1p.sx + l1v.sx), min(l1p.ex, l1p.ex + l1v.ex)), 
        min(min(l1p.sy, l1p.sy + l1v.sy), min(l1p.ey, l1p.ey + l1v.ey)), 
        max(max(l1p.sx, l1p.sx + l1v.sx), max(l1p.ex, l1p.ex + l1v.ex)), 
        max(max(l1p.sy, l1p.sy + l1v.sy), max(l1p.ey, l1p.ey + l1v.ey))
      ), Coord4(
        min(min(l2p.sx, l2p.sx + l2v.sx), min(l2p.ex, l2p.ex + l2v.ex)), 
        min(min(l2p.sy, l2p.sy + l2v.sy), min(l2p.ey, l2p.ey + l2v.ey)), 
        max(max(l2p.sx, l2p.sx + l2v.sx), max(l2p.ex, l2p.ex + l2v.ex)), 
        max(max(l2p.sy, l2p.sy + l2v.sy), max(l2p.ey, l2p.ey + l2v.ey))
      )
    ))
    return make_pair(NOCOLLIDE, Coord2());*/
  
  Coord cBc = 2;
  Coord2 pos;
  for(int i = 0; i < 4; i++) {
    const Coord4 *linepos;
    const Coord4 *linevel;
    Coord4 ptposvel;
    switch(i) {
      case 0:
        linepos = &l1p;
        linevel = &l1v;
        ptposvel = Coord4(l2p.sx, l2p.sy, l2v.sx, l2v.sy);
        break;
      case 1:
        linepos = &l1p;
        linevel = &l1v;
        ptposvel = Coord4(l2p.ex, l2p.ey, l2v.ex, l2v.ey);
        break;
      case 2:
        linepos = &l2p;
        linevel = &l2v;
        ptposvel = Coord4(l1p.sx, l1p.sy, l1v.sx, l1v.sy);
        break;
      case 3:
        linepos = &l2p;
        linevel = &l2v;
        ptposvel = Coord4(l1p.ex, l1p.ey, l1v.ex, l1v.ey);
        break;
      default:
        CHECK(0);
    }
    pair<Coord, Coord> tbv = getLineCollision(*linepos, *linevel, ptposvel);
    
    proc2(&cBc, &pos, &tbv, *linepos, *linevel, ptposvel);
  }
  CHECK(cBc != NOCOLLIDE);
  if(likely(cBc == 2))
    cBc = NOCOLLIDE;
  return make_pair(cBc, pos);
}

/*
class CollideOhgod {
  public:
  CollideOhgod() {
    vector<string> cst = tokenize("00000075fd82148a 000000048e254405 00000075dfb5fe8f 000000061f1e2ee6 | 00000004fe86a300 000000003d697936 00000004f98f4a01 00000000803da05b | 0000007899798000 0000000d1a456000 0000007899798000 000000001f9a7000 | 0000000000000000 0000000000000000 0000000000000000 0000000000000000", "|");
    vector<Coord4> tst;
    for(int i = 0; i < cst.size(); i++) {
      vector<string> tok = tokenize(cst[i], " ");
      tst.push_back(Coord4(coordFromRawstr(tok[0]), coordFromRawstr(tok[1]), coordFromRawstr(tok[2]), coordFromRawstr(tok[3])));
    }
    CHECK(doCollisionNN(tst[0], tst[1], tst[2], tst[3]).first != NOCOLLIDE);
  }
} cog;
*/

inline pair<Coord, Coord2> doCollisionNU(const Coord4 &l1p, const Coord4 &l1v, const Coord4 &l2p, const Coord4 &l2v) {
  return doCollisionNN(l1p, l1v, l2p, l2v); // this can probably be optimized
}

inline pair<Coord, Coord2> doCollisionNP(const Coord4 &l1p, const Coord4 &l1v, const Coord4 &ptpv) {
  /*if(!boxBoxIntersect(
      Coord4(
        min(min(l1p.sx, l1p.sx + l1v.sx), min(l1p.ex, l1p.ex + l1v.ex)), 
        min(min(l1p.sy, l1p.sy + l1v.sy), min(l1p.ey, l1p.ey + l1v.ey)), 
        max(max(l1p.sx, l1p.sx + l1v.sx), max(l1p.ex, l1p.ex + l1v.ex)), 
        max(max(l1p.sy, l1p.sy + l1v.sy), max(l1p.ey, l1p.ey + l1v.ey))
      ), Coord4(
        min(ptpv.sx, ptpv.sx + ptpv.ex),
        min(ptpv.sy, ptpv.sy + ptpv.ey),
        max(ptpv.sx, ptpv.sx + ptpv.ex),
        max(ptpv.sy, ptpv.sy + ptpv.ey)
      )
    ))
    return make_pair(NOCOLLIDE, Coord2());*/
  pair<Coord, Coord> tbv = getLineCollision(l1p, l1v, ptpv);
  pair<Coord, Coord2> rv(2, Coord2());
  proc2(&rv.first, &rv.second, &tbv, l1p, l1v, ptpv);
  if(rv.first == 2)
    rv.first = NOCOLLIDE;
  return rv;
}

inline pair<Coord, Coord2> doCollisionUP(const Coord4 &l1, const Coord2 &l2p, const Coord2 &l2v) {
  //return doCollisionNN(l1, Coord4(0, 0, 0, 0), Coord4(l2p, l2p), Coord4(l2v, l2v)); // this can probably be optimized
  /*if(!boxBoxIntersect(
      Coord4(
        min(l1.sx, l1.ex),
        min(l1.sy, l1.ey),
        max(l1.sx, l1.ex),
        max(l1.sy, l1.ey)
      ), Coord4(
        min(l2p.x, l2p.x + l2v.x),
        min(l2p.y, l2p.y + l2v.y),
        max(l2p.x, l2p.x + l2v.x),
        max(l2p.y, l2p.y + l2v.y)
      )
    ))
    return make_pair(NOCOLLIDE, Coord2());*/
  Coord pos = linelineintersectpos(Coord4(l2p, l2p + l2v), l1);
  if(pos == 2)
    return make_pair(NOCOLLIDE, Coord2());
  return make_pair(pos, l2p + l2v * pos);
}

inline int getCategoryCount(int players) {
  return players * (CGR_LAST - 1) + 1;
}
inline int getPlayerCount(int index) {
  CHECK(index % (CGR_LAST - 1) == 1);
  return index / (CGR_LAST - 1);
}
int getCategoryFromPlayers(int players, int category, int bucket) {
  return players * category + bucket;
}
pair<int, int> reverseCategoryFromPlayers(int players, int index) {
  return make_pair(index / players, index % players);
}
pair<int, int> reverseCategoryFromCC(int categorycount, int index) {
  return reverseCategoryFromPlayers(getPlayerCount(categorycount), index);
}

bool isPersistentFromPlayers(int players, int index) {
  int cat = reverseCategoryFromPlayers(players, index).first;
  return cat == CGR_STATPROJECTILE || cat == CGR_WALL;
}

bool canCollidePlayer(int players, int indexa, int indexb, const vector<int> &teams) {
  pair<int, int> ar = reverseCategoryFromPlayers(players, indexa);
  pair<int, int> br = reverseCategoryFromPlayers(players, indexb);
  // Two things can't collide if they're part of the same ownership group (ignoring walls which are different)
  if(ar.second == br.second && ar.first != CGR_WALL && br.first != CGR_WALL)
    return false;
  // Nothing can collide with itself
  if(ar == br)
    return false;
  // Projectiles just don't collide
  if(ar.first == CGR_PROJECTILE || br.first == CGR_PROJECTILE || ar.first == CGR_STATPROJECTILE || br.first == CGR_STATPROJECTILE || ar.first == CGR_NOINTPROJECTILE || br.first == CGR_NOINTPROJECTILE)
    return false;
  CHECK(ar.first == CGR_WALL || ar.first == CGR_TANK);
  CHECK(br.first == CGR_WALL || br.first == CGR_TANK);
  return true;
}
bool canCollideProjectile(int players, int indexa, int indexb, const vector<int> &teams) {
  pair<int, int> ar = reverseCategoryFromPlayers(players, indexa);
  pair<int, int> br = reverseCategoryFromPlayers(players, indexb);
  // Let's make things slightly easier.
  if(ar > br)
    swap(ar, br);
  // Two things can't collide if they're part of the same ownership group (ignoring walls which are different).
  if(ar.second == br.second && ar.first != CGR_WALL && br.first != CGR_WALL)
    return false;
  // Nothing can collide with itself.
  if(ar == br)
    return false;
  // Tanks can collide with projectiles.
  if(ar.first == CGR_TANK && br.first == CGR_PROJECTILE)
    return true;
  // And with mines, as long as they're not on the same team.
  if(ar.first == CGR_TANK && br.first == CGR_STATPROJECTILE && teams[ar.second] != teams[br.second])
    return true;
  // And with no-intersection projectiles, as long as they're not on the same team.
  if(ar.first == CGR_TANK && br.first == CGR_NOINTPROJECTILE && teams[ar.second] != teams[br.second])
    return true;
  // Projectiles can hit each other, as long as they're not on the same team.
  if(ar.first == CGR_PROJECTILE && br.first == CGR_PROJECTILE && teams[ar.second] != teams[br.second])
    return true;
  // Projectiles can hit walls.
  if(ar.first == CGR_PROJECTILE && br.first == CGR_WALL)
    return true;
  // Both kinds.
  if(ar.first == CGR_NOINTPROJECTILE && br.first == CGR_WALL)
    return true;
  // That's it. Easy!
  return false;
}

inline bool operator==(const CollideId &lhs, const CollideId &rhs) {
  if(lhs.category != rhs.category) return false;
  if(lhs.bucket != rhs.bucket) return false;
  if(lhs.item != rhs.item) return false;
  return true;
}

inline bool operator!=(const CollideId &lhs, const CollideId &rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const CollideData &lhs, const CollideData &rhs) {
  if(lhs.lhs != rhs.lhs) return lhs.lhs < rhs.lhs;
  if(lhs.rhs != rhs.rhs) return lhs.rhs < rhs.rhs;
  if(lhs.t != rhs.t) return lhs.t < rhs.t;
  return false;
}

inline bool operator==(const CollideData &lhs, const CollideData &rhs) {
  if(lhs.lhs != rhs.lhs) return false;
  if(lhs.rhs != rhs.rhs) return false;
  if(lhs.t != rhs.t) return false;
  return true;
}

CollidePiece::CollidePiece(const Coord4 &pos, const Coord4 &vel, int type) : pos(pos), vel(vel), type(type) {
  if(type == NORMAL) {
    bbx = Coord4(
      min(min(pos.sx, pos.sx + vel.sx), min(pos.ex, pos.ex + vel.ex)), 
      min(min(pos.sy, pos.sy + vel.sy), min(pos.ey, pos.ey + vel.ey)), 
      max(max(pos.sx, pos.sx + vel.sx), max(pos.ex, pos.ex + vel.ex)), 
      max(max(pos.sy, pos.sy + vel.sy), max(pos.ey, pos.ey + vel.ey))
    );
  } else if(type == UNMOVING) {
    CHECK(vel == Coord4(0, 0, 0, 0));
    bbx = Coord4(
      min(pos.sx, pos.ex),
      min(pos.sy, pos.ey),
      max(pos.sx, pos.ex),
      max(pos.sy, pos.ey)
    );
  } else if(type == POINT) {
    CHECK(pos.s() == pos.e());
    CHECK(vel.s() == vel.e());
    bbx = Coord4(
      min(pos.sx, pos.sx + vel.sx),
      min(pos.sy, pos.sy + vel.sy),
      max(pos.sx, pos.sx + vel.sx),
      max(pos.sy, pos.sy + vel.sy)
    );
  } else {
    CHECK(0);
  }
}

void CollideZone::makeSpaceFor(int id) {
  if(catrefs[id] == -1) {
    catrefs[id] = items.size();
    items.resize(items.size() + 1);
    items[catrefs[id]].first = id;
  }
}
void CollideZone::wipe(int id) {
  if(catrefs[id] != -1) {
    swap(items[catrefs[id]].first, items[items.size() - 1].first);
    items[catrefs[id]].second.swap(items[items.size() - 1].second);
    catrefs[items[catrefs[id]].first] = catrefs[id];
    catrefs[id] = -1;
    items.pop_back();
  }
}

void CollideZone::setCategoryCount(int size) {
  CHECK(!items.size());
  CHECK(!catrefs.size());
  catrefs.resize(size, -1);
}

void CollideZone::clean(const char *persist) {
  for(int i = 0; i < items.size(); i++) {
    if(!persist[items[i].first]) {
      wipe(items[i].first);
      i--;
    }
  }
}

void CollideZone::addToken(int category, int group, const CollidePiece &piece) {
  makeSpaceFor(category);
  items[catrefs[category]].second[group].push_back(piece);
}
void CollideZone::dumpGroup(int category, int group) {
  if(catrefs[category] != -1)
    items[catrefs[category]].second.erase(group);
}

bool CollideZone::checkSimpleCollision(int groupid, const vector<Coord4> &line, const Coord4 &bbox, const char *collidematrix) const {
  for(int i = 0; i < items.size(); i++) {
    if(!collidematrix[groupid * catrefs.size() + items[i].first])
      continue;
    for(map<int, vector<CollidePiece> >::const_iterator itr = items[i].second.begin(); itr != items[i].second.end(); ++itr) {
      const vector<CollidePiece> &tx = itr->second;
      for(int xa = 0; xa < tx.size(); xa++) {
        if(!boxLineIntersect(bbox, tx[xa].pos))
          continue;
        for(int ya = 0; ya < line.size(); ya++) {
          if(linelineintersect(tx[xa].pos, line[ya]))
            return true;
        }
      }
    }
  }
  return false;
}

int cp_nn = 0;
int cp_nu = 0;
int cp_np = 0;
int cp_uu = 0;
int cp_up = 0;
int cp_pp = 0;

void displayCZInfo() {
  dprintf("      NN: %d\n", cp_nn);
  dprintf("      NU: %d\n", cp_nu);
  dprintf("      NP: %d\n", cp_np);
  dprintf("      UU: %d\n", cp_uu);
  dprintf("      UP: %d\n", cp_up);
  dprintf("      PP: %d\n", cp_pp);
  cp_nn = 0;
  cp_nu = 0;
  cp_np = 0;
  cp_uu = 0;
  cp_up = 0;
  cp_pp = 0;
}

inline bool doCompare(const CollidePiece *tix, const CollidePiece *tiy, pair<Coord, Coord2> *rv) {
  if(unlikely(boxBoxIntersect(tix->bbx, tiy->bbx))) {
    if(tix->type > tiy->type)
      swap(tix, tiy);
    if(tix->type == CollidePiece::UNMOVING && tiy->type == CollidePiece::POINT) {
      cp_up++;
      *rv = doCollisionUP(tix->pos, tiy->pos.s(), tiy->vel.s());
    } else if(tix->type == CollidePiece::NORMAL && tiy->type == CollidePiece::UNMOVING) {
      cp_nu++;
      *rv = doCollisionNU(tix->pos, tix->vel, tiy->pos, tiy->vel);
    } else if(tix->type == CollidePiece::NORMAL && tiy->type == CollidePiece::NORMAL) {
      cp_nn++;
      *rv = doCollisionNN(tix->pos, tix->vel, tiy->pos, tiy->vel);
    } else if(tix->type == CollidePiece::NORMAL && tiy->type == CollidePiece::POINT) {
      cp_np++;
      *rv = doCollisionNP(tix->pos, tix->vel, Coord4(tiy->pos.s(), tiy->vel.s()));
    } else if(tix->type == CollidePiece::POINT && tiy->type == CollidePiece::POINT) {
      cp_pp++;
      CHECK(0); // no
    } else if(tix->type == CollidePiece::UNMOVING && tiy->type == CollidePiece::UNMOVING) {
      cp_uu++;
      CHECK(0); // no
    } else {
      CHECK(0);
    }
    return true;
  } else {
    return false;
  }
}

bool CollideZone::checkSingleCollision(int groupid, vector<pair<Coord, Coord> > *clds, const vector<CollidePiece> &cpiece, const char *collidematrix) const {
  pair<Coord, Coord2> cd;
  for(int i = 0; i < items.size(); i++) {
    if(!collidematrix[groupid * catrefs.size() + items[i].first])
      continue;
    for(map<int, vector<CollidePiece> >::const_iterator itr = items[i].second.begin(); itr != items[i].second.end(); ++itr) {
      const vector<CollidePiece> &tx = itr->second;
      for(int xa = 0; xa < tx.size(); xa++) {
        for(int ya = 0; ya < cpiece.size(); ya++) {
          if(unlikely(doCompare(&tx[xa], &cpiece[ya], &cd))) {
            if(unlikely(cd.first != NOCOLLIDE)) {
              CHECK(cd.first >= 0 && cd.first <= 1);
              //dprintf("%f %f\n", len(cd.second - lerp(tx[xa].pos.s(), tx[xa].pos.s() + tx[xa].vel.s(), cd.first)), len(cd.second - lerp(tx[xa].pos.e(), tx[xa].pos.e() + tx[xa].vel.e(), cd.first)));
              if(len(cd.second - lerp(tx[xa].pos.s(), tx[xa].pos.s() + tx[xa].vel.s(), cd.first)) < Coord(0.0001) || len(cd.second - lerp(tx[xa].pos.e(), tx[xa].pos.e() + tx[xa].vel.e(), cd.first)) < Coord(0.0001)) {
                clds->push_back(make_pair(cd.first, getAngle(lerp(cpiece[ya].pos, cpiece[ya].pos + cpiece[ya].vel, cd.first).vector()).toFloat() + PI / 2));
              } else {
                clds->push_back(make_pair(cd.first, getAngle(lerp(tx[xa].pos, tx[xa].pos + tx[xa].vel, cd.first).vector()).toFloat() + PI / 2));
              }
            }
          }
        }
      }
    }
  }
  return false;
}

void CollideZone::processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const {
  pair<Coord, Coord2> cd;
  for(int x = 0; x < items.size(); x++) {
    for(int y = x + 1; y < items.size(); y++) {
      if(!collidematrix[items[x].first * catrefs.size() + items[y].first])
        continue;
      for(map<int, vector<CollidePiece> >::const_iterator xitr = items[x].second.begin(); xitr != items[x].second.end(); ++xitr) {
        const vector<CollidePiece> &tx = xitr->second;
        for(map<int, vector<CollidePiece> >::const_iterator yitr = items[y].second.begin(); yitr != items[y].second.end(); ++yitr) {
          const vector<CollidePiece> &ty = yitr->second;
          for(int xa = 0; xa < tx.size(); xa++) {
            for(int ya = 0; ya < ty.size(); ya++) {
              if(unlikely(doCompare(&tx[xa], &ty[ya], &cd))) {
                if(unlikely(cd.first != NOCOLLIDE)) {
                  CHECK(cd.first >= 0 && cd.first <= 1);
                  clds->push_back(make_pair(cd.first, CollideData(CollideId(reverseCategoryFromCC(catrefs.size(), items[x].first), xitr->first), CollideId(reverseCategoryFromCC(catrefs.size(), items[y].first), yitr->first), cd.first, make_pair(getAngle(lerp(tx[xa].pos, tx[xa].pos + tx[xa].vel, cd.first).vector()).toFloat() + PI / 2, getAngle(lerp(ty[ya].pos, ty[ya].pos + ty[ya].vel, cd.first).vector()).toFloat() + PI / 2))));
                }
              }
            }
          }
        }
      }
    }
  }
}

void CollideZone::render() const {
  /*
  for(int i = 0; i < items.size(); i++)
    for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator itr = items[i].second.begin(); itr != items[i].second.end(); itr++)
      for(int j = 0; j < itr->second.size(); j++)
        drawLine(itr->second[j].first, 1);*/
}

void CollideZone::checksum(Adler32 *adl) const {
  adler(adl, items);
  adler(adl, catrefs);
}

void Collider::cleanup(int mode, const Coord4 &bounds, const vector<int> &teams) {
  CHECK(state == CSTA_WAITING);
  CHECK(teams.size() == players);
  
  Coord4 zbounds = snapToEnclosingGrid(bounds, resolution);
  
  int nsx = (zbounds.sx / resolution).toInt();
  int nsy = (zbounds.sy / resolution).toInt();
  int nex = (zbounds.ex / resolution).toInt();
  int ney = (zbounds.ey / resolution).toInt();
  if(nsx != sx || nsy != sy || nex != ex || ney != ey) {
    //dprintf("Rescaling!\n");
    {
      vector<CollideZone> ncz((nex - nsx) * (ney - nsy));
      for(int nx = nsx; nx < nex; nx++) {
        for(int ny = nsy; ny < ney; ny++) {
          if(nx >= sx && nx < ex && ny >= sy && ny < ey) {
            swap(zones[cmap(nx, ny)], ncz[(nx - nsx) * (ney - nsy) + ny - nsy]);
          } else {
            ncz[(nx - nsx) * (ney - nsy) + ny - nsy].setCategoryCount(getCategoryCount(players));
          }
        }
      }
      swap(ncz, zones);
    }
    
    sx = nsx;
    sy = nsy;
    ex = nex;
    ey = ney;
  }
  
  {
    vector<char> persist;
    for(int i = 0; i < getCategoryCount(players); i++)
      persist.push_back(isPersistentFromPlayers(players, i));
    for(int i = 0; i < zones.size(); i++)
      zones[i].clean(&persist[0]);
  }
  
  collidematrix.clear();
  
  const int catcount = getCategoryCount(players);
  if(mode == COM_PLAYER) {
    for(int i = 0; i < catcount; i++)
      for(int j = 0; j < catcount; j++)
        collidematrix.push_back(canCollidePlayer(players, i, j, teams));
  } else if(mode == COM_PROJECTILE) {
    for(int i = 0; i < catcount; i++)
      for(int j = 0; j < catcount; j++)
        collidematrix.push_back(canCollideProjectile(players, i, j, teams));
  } else {
    CHECK(0);
  }
  
  for(int i = 0; i < catcount; i++)
    for(int j = 0; j < catcount; j++)
      CHECK(collidematrix[i * catcount + j] == collidematrix[j * catcount + i]);
}

void Collider::addNormalToken(const CollideId &cid, const Coord4 &line, const Coord4 &direction) {
  CHECK(state == CSTA_WAITING);

  Coord4 area = startCBoundBox();
  addToBoundBox(&area, line.sx, line.sy);
  addToBoundBox(&area, line.ex, line.ey);
  addToBoundBox(&area, line.sx + direction.sx, line.sy + direction.sy);
  addToBoundBox(&area, line.ex + direction.ex, line.ey + direction.ey);
  area = snapToEnclosingGrid(area, resolution);
  int tsx = max((area.sx / resolution).toInt(), sx);
  int tsy = max((area.sy / resolution).toInt(), sy);
  int tex = min((area.ex / resolution).toInt(), ex);
  int tey = min((area.ey / resolution).toInt(), ey);
  
  /*if(!(tsx < ex && tsy < ey && tex >= sx && tey >= sy)) {
    dprintf("%d, %d, %d, %d\n", tsx, tsy, tex, tey);
    dprintf("%d, %d, %d, %d\n", sx, sy, ex, ey);
    dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
    dprintf("%f, %f, %f, %f\n", line.sx.toFloat(), line.sy.toFloat(), line.ex.toFloat(), line.ey.toFloat());
    dprintf("%f, %f, %f, %f\n", direction.sx.toFloat(), direction.sy.toFloat(), direction.ex.toFloat(), direction.ey.toFloat());
    dprintf("-----\n");
    dprintf("%s, %s, %s, %s\n", area.sx.rawstr().c_str(), area.sy.rawstr().c_str(), area.ex.rawstr().c_str(), area.ey.rawstr().c_str());
    dprintf("%s, %s, %s, %s\n", line.sx.rawstr().c_str(), line.sy.rawstr().c_str(), line.ex.rawstr().c_str(), line.ey.rawstr().c_str());
    dprintf("%s, %s, %s, %s\n", direction.sx.rawstr().c_str(), direction.sy.rawstr().c_str(), direction.ex.rawstr().c_str(), direction.ey.rawstr().c_str());
    dprintf("-----\n");
    dprintf("Area bounds: %f,%f %f,%f\n", (sx * resolution).toFloat(), (sy * resolution).toFloat(), (ex * resolution).toFloat(), (ey * resolution).toFloat());
    CHECK(0);
  }
  CHECK(tsx < ex && tsy < ey && tex >= sx && tey >= sy);*/
  
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      zones[cmap(x, y)].addToken(categ, cid.item, CollidePiece(line, direction, CollidePiece::NORMAL));
    
  addedTokens++;
}

void Collider::addUnmovingToken(const CollideId &cid, const Coord4 &line) {
  CHECK(state == CSTA_WAITING);

  Coord4 area = startCBoundBox();
  addToBoundBox(&area, line.sx, line.sy);
  addToBoundBox(&area, line.ex, line.ey);
  area = snapToEnclosingGrid(area, resolution);
  int tsx = max((area.sx / resolution).toInt(), sx);
  int tsy = max((area.sy / resolution).toInt(), sy);
  int tex = min((area.ex / resolution).toInt(), ex);
  int tey = min((area.ey / resolution).toInt(), ey);
  
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      zones[cmap(x, y)].addToken(categ, cid.item, CollidePiece(line, Coord4(0, 0, 0, 0), CollidePiece::UNMOVING));
    
  addedTokens++;
}

void Collider::addPointToken(const CollideId &cid, const Coord2 &line, const Coord2 &direction) {
  CHECK(state == CSTA_WAITING);

  Coord4 area = startCBoundBox();
  addToBoundBox(&area, line.x, line.y);
  addToBoundBox(&area, line.x + direction.x, line.y + direction.y);
  area = snapToEnclosingGrid(area, resolution);
  int tsx = max((area.sx / resolution).toInt(), sx);
  int tsy = max((area.sy / resolution).toInt(), sy);
  int tex = min((area.ex / resolution).toInt(), ex);
  int tey = min((area.ey / resolution).toInt(), ey);
  
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      zones[cmap(x, y)].addToken(categ, cid.item, CollidePiece(Coord4(line, line), Coord4(direction, direction), CollidePiece::POINT));
    
  addedTokens++;
}

void Collider::dumpGroup(const CollideId &cid) {
  CHECK(state == CSTA_WAITING);
  
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int i = 0; i < zones.size(); i++) // todo: Get bounds from the caller?
    zones[i].dumpGroup(categ, cid.item);
  //persistent.erase(make_pair(categ, cid.item));
}

int Collider::consumeAddedTokens() const {
  int adt = addedTokens;
  addedTokens = 0;
  return adt;
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
  area.sx -= resolution / 4;
  area.sy -= resolution / 4;
  area.ex += resolution / 4;
  area.ey += resolution / 4;
  area = snapToEnclosingGrid(area, resolution);
  int tsx = max((area.sx / resolution).toInt(), sx);
  int tsy = max((area.sy / resolution).toInt(), sy);
  int tex = min((area.ex / resolution).toInt(), ex);
  int tey = min((area.ey / resolution).toInt(), ey);
  CHECK(tsx < ex && tsy < ey && tex >= sx && tey >= sy);
  /*
  if(!(txs < zxe && tys < zye && txe > zxs && tye > zys)) {
    dprintf("%d, %d, %d, %d\n", txs, tys, txe, tye);
    dprintf("%d, %d, %d, %d\n", zxs, zys, zxe, zye);
    dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
    dprintf("%f, %f, %f, %f\n", pa.sx.toFloat(), pa.sy.toFloat(), pa.ex.toFloat(), pa.ey.toFloat());
    CHECK(0);
  }
  */
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      if(zones[cmap(x, y)].checkSimpleCollision(getCategoryFromPlayers(players, category, gid), line, pa, &*collidematrix.begin()))
        return true;
  return false;
}

bool Collider::checkSingleCollision(int category, int gid, const vector<Coord4> &line, const vector<Coord4> &delta, Coord *ang) const {
  CHECK(line.size() == delta.size());
  if(line.size() == 0)
    return false;
  Coord4 area = startCBoundBox();
  for(int i = 0; i < line.size(); i++) {
    addToBoundBox(&area, line[i].sx, line[i].sy);
    addToBoundBox(&area, line[i].ex, line[i].ey);
    addToBoundBox(&area, line[i].sx + delta[i].sx, line[i].sy + delta[i].sy);
    addToBoundBox(&area, line[i].ex + delta[i].ex, line[i].ey + delta[i].ey);
  }
  Coord4 pa = area;
  area.sx -= resolution / 4;
  area.sy -= resolution / 4;
  area.ex += resolution / 4;
  area.ey += resolution / 4;
  area = snapToEnclosingGrid(area, resolution);
  int tsx = max((area.sx / resolution).toInt(), sx);
  int tsy = max((area.sy / resolution).toInt(), sy);
  int tex = min((area.ex / resolution).toInt(), ex);
  int tey = min((area.ey / resolution).toInt(), ey);
  CHECK(tsx < ex && tsy < ey && tex >= sx && tey >= sy);
  /*
  if(!(txs < zxe && tys < zye && txe > zxs && tye > zys)) {
    dprintf("%d, %d, %d, %d\n", txs, tys, txe, tye);
    dprintf("%d, %d, %d, %d\n", zxs, zys, zxe, zye);
    dprintf("%f, %f, %f, %f\n", area.sx.toFloat(), area.sy.toFloat(), area.ex.toFloat(), area.ey.toFloat());
    dprintf("%f, %f, %f, %f\n", pa.sx.toFloat(), pa.sy.toFloat(), pa.ex.toFloat(), pa.ey.toFloat());
    CHECK(0);
  }
  */
  
  vector<CollidePiece> cdat;
  for(int i = 0; i < line.size(); i++)
    cdat.push_back(CollidePiece(line[i], delta[i], CollidePiece::NORMAL));
  
  vector<pair<Coord, Coord> > clds;
  
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      zones[cmap(x, y)].checkSingleCollision(getCategoryFromPlayers(players, category, gid), &clds, cdat, &*collidematrix.begin());
  if(!clds.size())
    return false;
  
  if(ang)
    *ang = min_element(clds.begin(), clds.end())->second;
  return true;
}

void Collider::processMotion() {
  CHECK(state == CSTA_WAITING);
  state = CSTA_PROCESSED;
  collides.clear();
  curcollide = -1;
  
  vector<pair<Coord, CollideData> > clds;
  
  // TODO: Don't bother processing unique pairs more than once?
  for(int i = 0; i < zones.size(); i++)
    zones[i].processMotion(&clds, &*collidematrix.begin());
  
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

void Collider::finishProcess() {
  CHECK(state == CSTA_PROCESSED);
  state = CSTA_WAITING;
}

Collider::Collider(int players, Coord resolution) : players(players), resolution(resolution), state(CSTA_WAITING), sx(0), ex(0), sy(0), ey(0) { };
Collider::~Collider() {
  sx = 1234;
  sy = 5838;
  ex = -3282;
  ey = -5983; // for checksumming
};

DECLARE_bool(debugGraphics);
void Collider::render() const {
}
void Collider::renderAround(const Coord2 &kord) const {
  if(FLAGS_debugGraphics) {
    setColor(Color(1.0, 0.4, 0.4));
    int x = floor(kord.x / resolution).toInt();
    int y = floor(kord.y / resolution).toInt();
    for(int dx = -1; dx <= 1; dx++) {
      for(int dy = -1; dy <= 1; dy++) {
        int tx = x + dx;
        int ty = y + dy;
        if(tx < sx || ty < sy || tx >= ex || ty >= ey)
          continue;
        zones[cmap(tx, ty)].render();
        drawText(StringPrintf("%d", cmap(tx, ty)), 3, Coord2(tx * resolution, ty * resolution).toFloat());
      }
    }
  }
}

void Collider::checksum(Adler32 *adl) const {
  adler(adl, players);
  adler(adl, resolution);
  adler(adl, state);
  adler(adl, collidematrix);
  adler(adl, sx);
  adler(adl, ex);
  adler(adl, sy);
  adler(adl, ey);
  adler(adl, zones);
}

void adler(Adler32 *adl, const CollidePiece &collider) {
  adler(adl, collider.type);
  adler(adl, collider.pos);
  adler(adl, collider.vel);
  adler(adl, collider.bbx);
}
