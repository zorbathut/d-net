#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include "coord.h"

#include <map>
#include <set>

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
  Coord2 pos;

  CollideData() { };
  CollideData(const CollideId &lhs, const CollideId &rhs, const Coord2 &pos) : lhs(lhs), rhs(rhs), pos(pos) { };
};

class CollideZone {
private:
  vector<map<int, vector<pair<Coord4, Coord4> > > > items;

public:
  
  void setCategoryCount(int size);

  void clean(const set<pair<int, int> > &persistent);

  void addToken(int category, int group, const Coord4 &line, const Coord4 &direction);
  void dumpGroup(int category, int group);

  bool checkSimpleCollision(int groupid, const vector<Coord4> &line, const char *collidematrix) const;

  void processSimple(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const;
  void processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const;

  void render() const;
};

enum {COM_PLAYER, COM_PROJECTILE};
enum {CGR_TANK, CGR_PROJECTILE, CGR_STATPROJECTILE, CGR_WALL};
const int CGR_WALLOWNER = 0;

class Collider {
public:

  void cleanup(int mode, const Coord4 &bounds, const vector<int> &teams);

  void addToken(const CollideId &cid, const Coord4 &line, const Coord4 &direction);
  void markPersistent(const CollideId &cid);
  void dumpGroup(const CollideId &cid);

  bool checkSimpleCollision(int category, int gid, const vector<Coord4> &line) const;

  void processSimple();
  void processMotion();

  bool next();
  const CollideData &getCollision() const;

  void finishProcess();

  Collider(int players, Coord resolution);
  ~Collider();

  void render() const;
  void renderAround(const Coord2 &kord) const;

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
  set<pair<int, int> > persistent;
  
  // Output collides
  vector<CollideData> collides;
  int curcollide;
};

#endif
