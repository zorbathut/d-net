#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
using namespace std;

class Collider {
public:

	void add( float sx, float sy, float ex, float ey );
	void remove( float sx, float sy, float ex, float ey );
	bool test( float sx, float sy, float ex, float ey ) const;

private:

	vector< float > verts;

};

#endif
