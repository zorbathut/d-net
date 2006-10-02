#ifndef DNET_GAMEMAP
#define DNET_GAMEMAP

#include "level.h"

using namespace std;

class Collider;

class Gamemap {
public:

  void render() const;
  void updateCollide(Collider *collide);

  Coord4 getBounds() const;

  void removeWalls(Coord2 center, float radius);

  void checkConsistency() const;
  
  Gamemap();
  Gamemap(const Level &level);

private:
  int sx;
  int sy;
  int ex;
  int ey;

  Coord4 getInternalBounds() const;
  Coord4 getTileBounds(int x, int y) const;

  vector<pair<int, vector<Coord2> > > paths;

};

#endif
