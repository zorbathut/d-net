#ifndef DNET_GAMEMAP
#define DNET_GAMEMAP

#include "collide.h"

#include <vector>
using namespace std;

class Gamemap {
public:

	void render() const;
	void addCollide( Collider *collide ) const;

	Gamemap();

private:

	vector< float > vertices;

};

#endif
