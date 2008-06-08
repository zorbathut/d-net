#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include "coord.h"

#include <map>

using namespace std;

#define ENABLE_COLLIDE_DEBUG_VIS

struct CollideId {
public:
  int category;
  int bucket;
  int item;

  CollideId() { };
  CollideId(int category, int bucket, int item) : category(category), bucket(bucket), item(item) { };
  CollideId(pair<int, int> catbuck, int item) : category(catbuck.first), bucket(catbuck.second), item(item) { };
  CollideId(const CollideId &rhs) : category(rhs.category), bucket(rhs.bucket), item(rhs.item) { };
};

inline bool operator<(const CollideId &lhs, const CollideId &rhs) {
  if(lhs.category != rhs.category) return lhs.category < rhs.category;
  if(lhs.bucket != rhs.bucket) return lhs.bucket < rhs.bucket;
  if(lhs.item != rhs.item) return lhs.item < rhs.item;
  return false;
}

struct CollideData {
public:
  CollideId lhs;
  CollideId rhs;
  Coord t;
  pair<Coord, Coord> normals;

  CollideData() { };
  CollideData(const CollideId &lhs, const CollideId &rhs, const Coord &t, pair<Coord, Coord> normals) : lhs(lhs), rhs(rhs), t(t), normals(normals) { };
};

class CollidePiece {
public:
  enum { NORMAL, UNMOVING, POINT };
  Coord4 pos;
  Coord4 vel;
  int type;
  
  Coord4 bbx;
  
  CollidePiece() { };
  CollidePiece(const Coord4 &pos, const Coord4 &vel, int type);
  CollidePiece(const CollidePiece &piece) : pos(piece.pos), vel(piece.vel), type(piece.type), bbx(piece.bbx) { };
};

class CollideZone {
private:
  vector<pair<int, map<int, vector<CollidePiece> > > > items;
  vector<int> catrefs;

  void makeSpaceFor(int id);
  void wipe(int id);

public:
  
  void setCategoryCount(int size);

  void clean(const char *persist);

  void addToken(int category, int group, const CollidePiece &piece);
  void dumpGroup(int category, int group);

  bool checkSimpleCollision(int groupid, const vector<Coord4> &line, const Coord4 &bbox, const char *collidematrix) const;
  bool checkSingleCollision(int groupid, vector<pair<Coord, Coord> > *clds, const vector<CollidePiece> &cpiece, const char *collidematrix) const;

  void processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const;

  void render() const;

  void checksum(Adler32 *adl) const;
};

enum {COM_PLAYER, COM_PROJECTILE};
enum {CGR_TANK, CGR_PROJECTILE, CGR_STATPROJECTILE, CGR_NOINTPROJECTILE, CGR_WALL, CGR_LAST};
const int CGR_WALLOWNER = 0;

class Collider {
public:

  void cleanup(int mode, const Coord4 &bounds, const vector<int> &teams);

  void addNormalToken(const CollideId &cid, const Coord4 &line, const Coord4 &direction);
  void addUnmovingToken(const CollideId &cid, const Coord4 &line);
  void addPointToken(const CollideId &cid, const Coord2 &pos, const Coord2 &vel);

  void dumpGroup(const CollideId &cid);

  int consumeAddedTokens() const;

  bool checkSimpleCollision(int category, int gid, const vector<Coord4> &line) const;
  bool checkSingleCollision(int category, int gid, const vector<Coord4> &lines, const vector<Coord4> &deltas, Coord *ang) const;

  void processMotion();

  bool next() {
    CHECK(state == CSTA_PROCESSED);
    //CHECK(curcollide == -1 || curcollide >= 0 && curcollide < collides.size());
    curcollide++;
    return curcollide < collides.size();
  }
  const CollideData &getCollision() const {
    CHECK(state == CSTA_PROCESSED);
    return collides[curcollide];
  }
  
  void finishProcess();

  Collider(int players, Coord resolution);
  ~Collider();

  void render() const;
  void renderAround(const Coord2 &kord) const;
  
  void checksum(Adler32 *adl) const;

private:
  
  enum { CSTA_WAITING, CSTA_PROCESSED };
  
  // These probably shouldn't ever change
  int players;
  Coord resolution;
  
  // State and per-process stuff
  int state;
  vector<char> collidematrix;
  
  // Zone data and persistent flags
  int sx, ex, sy, ey;
  inline int cmap(int x, int y) const { return (x - sx) * (ey - sy) + y - sy; };
  vector<CollideZone> zones;
  
  // Output collides
  vector<CollideData> collides;
  int curcollide;
  
  // Debug stuff
  mutable int addedTokens;
};

inline void adler(Adler32 *adl, const Collider &collider) { collider.checksum(adl); }
inline void adler(Adler32 *adl, const CollideZone &collider) { collider.checksum(adl); }
void adler(Adler32 *adl, const CollidePiece &collider);

#endif
