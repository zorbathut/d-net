#ifndef DNET_GAMEMAP
#define DNET_GAMEMAP

#include "collide.h"
#include "level.h"
#include "coord.h"

#include <vector>

using namespace std;

class Gamemap {
public:

  void render() const;
  void updateCollide( Collider *collide );

  //const vector<vector<Coord2> > &getCollide() const;

  Coord4 getBounds() const;

  void removeWalls(Coord2 center, float radius);

  void checkConsistency() const;
  
  Gamemap();
  Gamemap(const Level &level);

private:

  vector<pair<int, vector<Coord2> > > paths;

};

#endif
