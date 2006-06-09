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

  vector<pair<int, vector<Coord2> > > paths;

};

#endif
