#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
using namespace std;

class Collider {
public:

	void startGroup();
	void add( float sx, float sy, float ex, float ey );
	int endGroup();

	void disableGroup( int group );
	void enableGroup( int group );
	
	bool test( float sx, float sy, float ex, float ey ) const;

	Collider();

private:

	vector< float > verts;
	vector< int > groups;
	vector< bool > active;

};

#endif
