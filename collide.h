#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
#include <map>
#include <set>
using namespace std;

#include "util.h"
#include "coord.h"

#define ENABLE_COLLIDE_DEBUG_VIS

struct CollideId {
public:
  int category;
  int bucket;
  int item;

  CollideId() { };
  CollideId(pair<int, int> catbuck, int in_item) : category(catbuck.first), bucket(catbuck.second), item(in_item) { };
  CollideId(const CollideId &rhs) : category(rhs.category), bucket(rhs.bucket), item(rhs.item) { };
};

inline bool operator<(const CollideId &lhs, const CollideId &rhs) {
  if(lhs.category != rhs.category) return lhs.category < rhs.category;
  if(lhs.bucket != rhs.bucket) return lhs.bucket < rhs.bucket;
  if(lhs.item != rhs.item) return lhs.item < rhs.item;
  return false;
}
inline bool operator>(const CollideId &lhs, const CollideId &rhs) {
  return rhs < lhs;
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

struct CollideData {
public:
  CollideId lhs;
  CollideId rhs;
  Coord2 loc;

  CollideData() { };
  CollideData(const CollideId &in_lhs, const CollideId &in_rhs, const Coord2 &in_loc) : lhs(in_lhs), rhs(in_rhs), loc(in_loc) { };
  CollideData(const CollideData &in_rhs) : lhs(in_rhs.lhs), rhs(in_rhs.rhs), loc(in_rhs.loc) { };
};

inline bool operator<(const CollideData &lhs, const CollideData &rhs) {
  if(lhs.lhs != rhs.lhs) return lhs.lhs < rhs.lhs;
  if(lhs.rhs != rhs.rhs) return lhs.rhs < rhs.rhs;
  if(lhs.loc != rhs.loc) return lhs.loc < rhs.loc;
  return false;
}
inline bool operator>(const CollideData &lhs, const CollideData &rhs) {
  return rhs < lhs;
}

inline bool operator==(const CollideData &lhs, const CollideData &rhs) {
  if(lhs.lhs != rhs.lhs) return false;
  if(lhs.rhs != rhs.rhs) return false;
  if(lhs.loc != rhs.loc) return false;
  return true;
}

class ColliderZone {
private:
  vector< pair< int, vector< pair< int, pair< Coord4, Coord4 > > > > > items;
  int lastItem;

  int players;
public:

  void addToken(int groupid, int token, const Coord4 &line, const Coord4 &direction);
  void clearToken(int groupid, int token);

  void clearGroup(int groupid);

  bool checkSimpleCollision(int groupid, const vector<Coord4> &line, const char *collidematrix) const;

  void processSimple(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const;
  void processMotion(vector<pair<Coord, CollideData> > *clds, const char *collidematrix) const;

  void render(const Coord4 &bbox) const;

  void reset(int wallid);
  void full_reset();

  ColliderZone();
  ColliderZone(int players);
};

enum {COM_PLAYER, COM_PROJECTILE};
enum {CGR_WALL, CGR_PLAYER, CGR_PROJECTILE};

class Collider {
public:

  void resetNonwalls(int mode, const Coord4 &bounds, const vector<int> &teams);
  bool consumeFullReset();

  void startToken( int toki );
  void token(const Coord4 &line, const Coord4 &direction);
  void token(const Coord4 &line);
  void clearToken(int tokid);

  void addThingsToGroup(int category, int gid, bool log = false);
  void endAddThingsToGroup();

  void clearGroup(int category, int gid);

  bool checkSimpleCollision(int category, int gid, const vector<Coord4> &line) const;

  void processSimple();
  void processMotion();

  bool next();
  const CollideData &getData() const;

  void finishProcess();

  Collider();
  Collider(int players);
  ~Collider();

  void render() const;

private:
  
  enum { CSTA_UNINITTED, CSTA_WAIT, CSTA_ADD, CSTA_PROCESSED };
  
  int state;
  bool log;
  
  bool full_reset;

  int curcollide;
  
  int zxs;
  int zys;
  int zxe;
  int zye;
  vector<vector<ColliderZone> > zone;
  
  vector< CollideData > collides;

  int curpush;
  int curtoken;

  int players;
  
  vector<char> collidematrix;

};

#endif
