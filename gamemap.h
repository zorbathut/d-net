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

    vector<Coord4> getCollide() const;

    Coord4 getBounds() const;

    Gamemap();
	Gamemap(const Level &level);

private:

	vector<Coord4> paths;

};

#endif
