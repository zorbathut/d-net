
#include "collide.h"

#include "args.h"
#include "gfx.h"
#include "util.h"

#include <set>

using namespace std;

DECLARE_bool(verboseCollisions);

const Coord NOCOLLIDE = Coord(-1000000000);

pair<Coord, Coord> getLineCollision(const Coord4 &linepos, const Coord4 &linevel, const Coord4 &ptposvel) {
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
  //dprintf("a is %f, b is %f, c is %f\n", a, b, c);
  Coord sqrii = b * b - 4 * a * c;
  //dprintf("sqrii is %f\n", sqrii);
  Coord a2 = 2 * a;
  //dprintf("a2 is %f\n", a2);
  if(sqrii < 0)
    return make_pair(NOCOLLIDE, NOCOLLIDE);
  pair< Coord, Coord > rv;
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
  return ((x3 - x1) * (x2 - x1) + (y3 - y1) * (y2 - y1)) / ( sqr( x2 - x1) + sqr( y2 - y1));
}

pair<Coord, Coord2> getCollision(const Coord4 &l1p, const Coord4 &l1v, const Coord4 &l2p, const Coord4 &l2v) {
  Coord cBc = NOCOLLIDE;
  Coord2 pos;
  Coord4 temp;
  for(int i = 0; i < 4; i++) {
    const Coord4 *linepos;
    const Coord4 *linevel;
    const Coord4 *ptposvel;
    switch(i) {
      case 0:
        linepos = &l1p;
        linevel = &l1v;
        temp = Coord4(l2p.sx, l2p.sy, l2v.sx, l2v.sy);
        ptposvel = &temp;
        break;
      case 1:
        linepos = &l1p;
        linevel = &l1v;
        temp = Coord4(l2p.ex, l2p.ey, l2v.ex, l2v.ey);
        ptposvel = &temp;
        break;
      case 2:
        linepos = &l2p;
        linevel = &l2v;
        temp = Coord4(l1p.sx, l1p.sy, l1v.sx, l1v.sy);
        ptposvel = &temp;
        break;
      case 3:
        linepos = &l2p;
        linevel = &l2v;
        temp = Coord4(l1p.ex, l1p.ey, l1v.ex, l1v.ey);
        ptposvel = &temp;
        break;
      default:
        CHECK(0);
    }
    pair< Coord, Coord > tbv = getLineCollision(*linepos, *linevel, *ptposvel);
    Coord2 tpos;
    for(int j = 0; j < 2; j++) {
      Coord tt;
      if(j) {
        tt = tbv.second;
      } else {
        tt = tbv.first;
      }
      //if(verbosified && tt != NOCOLLIDE)
        //dprintf("%d, %d is %f\n", i, j, tt);
      if(tt < 0 || tt > 1 || (cBc != NOCOLLIDE && tt > cBc))
        continue;
      Coord u = getu(*linepos, *linevel, *ptposvel, tt);
      if(u < 0 || u > 1)
        continue;
      cBc = tt;
      Coord4 cline = *linepos + *linevel * tt;
      pos = Coord2(cline.sx, cline.sy) + Coord2(cline.ex - cline.sx, cline.ey - cline.sy) * u;
    }
  }
  return make_pair(cBc, pos);
}

inline int getCategoryCount(int players) {
  return players * 2 + 1;
}
inline int getPlayerCount(int index) {
  CHECK(index % 2);
  return index / 2;
}
int getCategoryFromPlayers(int players, int category, int bucket) {
  if(category == CGR_WALL) {
    CHECK(bucket == CGR_WALLOWNER);
    return 0;
  } else if(category == CGR_PLAYER) {
    CHECK(bucket >= 0 && bucket < players);
    return bucket + 1;
  } else if(category == CGR_PROJECTILE) {
    CHECK(bucket >= 0 && bucket < players);
    return players + bucket + 1;
  } else {
    return 0;
  }
}
pair<int, int> reverseCategoryFromPlayers(int players, int index) {
  if(index == 0)
    return make_pair(int(CGR_WALL), CGR_WALLOWNER);
  else if(index < players + 1)
    return make_pair(int(CGR_PLAYER), index - 1);
  else if(index < players * 2 + 1)
    return make_pair(int(CGR_PROJECTILE), index - players - 1);
  else
    CHECK(0);
}
pair<int, int> reverseCategoryFromPC(int playercount, int index) {
  return reverseCategoryFromPlayers(getPlayerCount(playercount), index);
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
  // Projectiles on the same team don't collide
  if(ar.first == CGR_PROJECTILE && br.first == CGR_PROJECTILE && teams[ar.second] == teams[br.second])
    return false;
  return true;
}
bool canCollideProjectile(int players, int indexa, int indexb, const vector<int> &teams) {
  pair<int, int> ar = reverseCategoryFromPlayers(players, indexa);
  pair<int, int> br = reverseCategoryFromPlayers(players, indexb);
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
  if(lhs.pos != rhs.pos) return lhs.pos < rhs.pos;
  return false;
}

inline bool operator==(const CollideData &lhs, const CollideData &rhs) {
  if(lhs.lhs != rhs.lhs) return false;
  if(lhs.rhs != rhs.rhs) return false;
  if(lhs.pos != rhs.pos) return false;
  return true;
}

void CollideZone::setCategoryCount(int size) {
  CHECK(!items.size());
  items.resize(size);
}

void CollideZone::clean(const set<pair<int, int> > &persistent) {
  CHECK(items.size());
  for(int i = 0; i < items.size(); i++) {
    for(map<int, vector<pair<Coord4, Coord4> > >::iterator itr = items[i].begin(); itr != items[i].end(); ) {
      if(persistent.count(make_pair(i, itr->first))) {
        itr++;
      } else {
        map<int, vector<pair<Coord4, Coord4> > >::iterator titr = itr;
        itr++;
        items[i].erase(titr);
      }
    }
  }
}

void CollideZone::addToken(int groupid, int token, const Coord4 &line, const Coord4 &direction) {
  items[groupid][token].push_back(make_pair(line, direction));
}
void CollideZone::dumpGroup(int category, int group) {
  items[category].erase(group);
}

bool CollideZone::checkSimpleCollision(int groupid, const vector<Coord4> &line, const char *collidematrix) const {
  for(int i = 0; i < items.size(); i++) {
    if(!collidematrix[groupid * items.size() + i])
      continue;
    for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator itr = items[i].begin(); itr != items[i].end(); ++itr) {
      const vector<pair<Coord4, Coord4> > &tx = itr->second;
      for(int xa = 0; xa < tx.size(); xa++) {
        for(int ya = 0; ya < line.size(); ya++) {
          if(linelineintersect(tx[xa].first, line[ya]))
            return true;
        }
      }
    }
  }
  return false;
}

void CollideZone::processSimple(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const {
  for(int x = 0; x < items.size(); x++) {
    for(int y = x + 1; y < items.size(); y++) {
      if(!collidematrix[x * items.size() + y])
        continue;
      for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator xitr = items[x].begin(); xitr != items[x].end(); ++xitr) {
        for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator yitr = items[y].begin(); yitr != items[y].end(); ++yitr) {
          const vector<pair<Coord4, Coord4> > &tx = xitr->second;
          const vector<pair<Coord4, Coord4> > &ty = yitr->second;
          for(int xa = 0; xa < tx.size(); xa++) {
            for(int ya = 0; ya < ty.size(); ya++) {
              if(linelineintersect(tx[xa].first, ty[ya].first)) {
                clds->push_back(make_pair(0, CollideData(CollideId(reverseCategoryFromPC(items.size(), x), xitr->first), CollideId(reverseCategoryFromPC(items.size(), y), yitr->first), Coord2())));
              }
            }
          }
        }
      }
    }
  }
}
void CollideZone::processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const {
  for(int x = 0; x < items.size(); x++) {
    for(int y = x + 1; y < items.size(); y++) {
      if(!collidematrix[x * items.size() + y])
        continue;
      for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator xitr = items[x].begin(); xitr != items[x].end(); ++xitr) {
        for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator yitr = items[y].begin(); yitr != items[y].end(); ++yitr) {
          const vector<pair<Coord4, Coord4> > &tx = xitr->second;
          const vector<pair<Coord4, Coord4> > &ty = yitr->second;
          for(int xa = 0; xa < tx.size(); xa++) {
            for(int ya = 0; ya < ty.size(); ya++) {
              pair<Coord, Coord2> tcol = getCollision(tx[xa].first, tx[xa].second, ty[ya].first, ty[ya].second);
              if(tcol.first == NOCOLLIDE)
                continue;
              CHECK(tcol.first >= 0 && tcol.first <= 1);
              clds->push_back(make_pair(tcol.first, CollideData(CollideId(reverseCategoryFromPC(items.size(), x), xitr->first), CollideId(reverseCategoryFromPC(items.size(), y), yitr->first), tcol.second)));
            }
          }
        }
      }
    }
  }
}

void CollideZone::render() const {
  for(int i = 0; i < items.size(); i++)
    for(map<int, vector<pair<Coord4, Coord4> > >::const_iterator itr = items[i].begin(); itr != items[i].end(); itr++)
      for(int j = 0; j < itr->second.size(); j++)
        drawLine(itr->second[j].first, 1);
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
    dprintf("Rescaling!\n");
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
  
  for(int i = 0; i < zones.size(); i++)
    zones[i].clean(persistent);
  
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
}

void Collider::addToken(const CollideId &cid, const Coord4 &line, const Coord4 &direction) {
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
  CHECK(tsx < ex && tsy < ey && tex >= sx && tey >= sy);
  /*
  if(!(tsx < sx && tsy < sy && tex > ex && tey > ey)) {
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
  */
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int x = tsx; x < tex; x++)
    for(int y = tsy; y < tey; y++)
      zones[cmap(x, y)].addToken(categ, cid.item, line, direction);
}

void Collider::markPersistent(const CollideId &cid) {
  CHECK(state == CSTA_WAITING);
  
  pair<int, int> tid(getCategoryFromPlayers(players, cid.category, cid.bucket), cid.item);
  CHECK(!persistent.count(tid));
  persistent.insert(tid);
}
bool Collider::isPersistent(const CollideId &cid) {
  pair<int, int> tid(getCategoryFromPlayers(players, cid.category, cid.bucket), cid.item);
  return persistent.count(tid);
}
void Collider::dumpGroup(const CollideId &cid) {
  CHECK(state == CSTA_WAITING);
  
  int categ = getCategoryFromPlayers(players, cid.category, cid.bucket);
  for(int i = 0; i < zones.size(); i++)
    zones[i].dumpGroup(categ, cid.item);
  persistent.erase(make_pair(categ, cid.item));
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
      if(zones[cmap(x, y)].checkSimpleCollision(getCategoryFromPlayers(players, category, gid), line, &*collidematrix.begin()))
        return true;
  return false;
}

void Collider::processSimple() {
  CHECK(state == CSTA_WAITING);
  state = CSTA_PROCESSED;
  collides.clear();
  curcollide = -1;
  
  vector<pair<Coord, CollideData> > clds;
  
  // TODO: Don't bother processing unique pairs more than once?
  for(int i = 0; i < zones.size(); i++)
    zones[i].processSimple(&clds, &*collidematrix.begin());
  
  sort(clds.begin(), clds.end());
  clds.erase(unique(clds.begin(), clds.end()), clds.end());
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

bool Collider::next() {
  CHECK(state == CSTA_PROCESSED);
  CHECK(curcollide == -1 || curcollide >= 0 && curcollide < collides.size());
  curcollide++;
  return curcollide < collides.size();
}
  
const CollideData &Collider::getCollision() const {
  CHECK(state == CSTA_PROCESSED);
  CHECK(curcollide >= 0 && curcollide < collides.size());
  return collides[curcollide];
}

void Collider::finishProcess() {
  CHECK(state == CSTA_PROCESSED);
  state = CSTA_WAITING;
}

Collider::Collider(int players, Coord resolution) : players(players), resolution(resolution), state(CSTA_WAITING), sx(0), ex(0), sy(0), ey(0) { };
Collider::~Collider() { };

DECLARE_bool(debugGraphics);
void Collider::render() const {
}
void Collider::renderAround(const Coord2 &kord) const {
  if(FLAGS_debugGraphics) {
    setColor(Color(1.0, 0.4, 0.4));
    int x = int(floor((kord.x / resolution).toFloat()));
    int y = int(floor((kord.y / resolution).toFloat()));
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
