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
  
  Gamemap();
  Gamemap(const Level &level);

private:
  int sx;
  int sy;
  int ex;
  int ey;

  Coord4 getInternalBounds() const;
  Coord4 getTileBounds(int x, int y) const;

  int linkid(int x, int y) const;
  void removePath(int x, int y, int id);
  int addPath(int x, int y);

  void flushAdds();

  vector<vector<int> > links;
  vector<pair<int, int> > nlinks;

  vector<int> available;
  vector<pair<int, vector<Coord2> > > paths;

};

#endif
