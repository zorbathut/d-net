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
  void addCollide( Collider *collide ) const;

  const vector<vector<Coord2> > &getCollide() const;

  Coord4 getBounds() const;

  void removeWalls(Coord2 center, float radius);

  Gamemap();
  Gamemap(const Level &level);

private:

  vector<vector<Coord2> > paths;

};

#endif
