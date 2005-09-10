#ifndef DNET_GAMEMAP
#define DNET_GAMEMAP

#include "collide.h"
#include "level.h"

#include <vector>

using namespace std;

class Gamemap {
public:

	void render() const;
	void addCollide( Collider *collide ) const;

    Float4 getBounds() const;

    Gamemap();
	Gamemap(const Level &level);

private:

	vector<vector<Float2> > paths;

};

#endif
