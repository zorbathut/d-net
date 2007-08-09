#ifndef DNET_GAMEMAP
#define DNET_GAMEMAP

#include "level.h"
#include "rng.h"

using namespace std;

class Collider;

class Gamemap {
public:

  void render() const;
  void updateCollide(Collider *collide);

  void checksum(Adler32 *adl) const;

  Coord4 getRenderBounds() const;
  Coord4 getCollisionBounds() const;

  void removeWalls(Coord2 center, float radius, Rng *rng);

  bool isInsideWall(Coord2 point) const;
  
  Gamemap();
  Gamemap(const vector<vector<Coord2> > &level, bool smashable);

private:
  bool smashable;

  int sx;
  int sy;
  int ex;
  int ey;

  Coord4 render_bounds;

  Coord4 getInternalBounds() const;
  Coord4 getTileBounds(int x, int y) const;

  int linkid(int x, int y) const;
  void removePath(int x, int y, int id);
  int addPath(int x, int y);

  void flushAdds();

  vector<vector<int> > links;
  vector<pair<int, int> > nlinks;

  struct Pathchunk {
    int state;
    vector<Coord2> collisionpath;
    vector<vector<Coord2> > renderpath;
    void generateRenderPath();
  };

  vector<int> available;
  vector<Pathchunk> paths;

};

#endif
