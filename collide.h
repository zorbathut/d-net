#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
#include <map>
using namespace std;

#include "util.h"

#define ENABLE_COLLIDE_DEBUG_VIS

class Collide {
public:
	Float4 line;
	int group;
	int sid;
	Collide();
	Collide( const Float4 &line, int sid );
};

class Quad {
public:
	Quad *quads;
	vector< Collide * > lines;
	Float4 range;
	bool sludge;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	float r, g, b;
#endif
	Quad();
	Quad( const Float4 &dim );
	~Quad();
};

class Collider {
public:

	void startGroup();
	void add( float sx, float sy, float ex, float ey );
	int endGroup();

	void disableGroup( int group );
	void enableGroup( int group );
	void deleteGroup( int group );
	
	bool test( float sx, float sy, float ex, float ey ) const;

	Collider();	// doesn't init to a usable state
	void reinit( float sx, float sy, float ex, float ey );
	Collider( float sx, float sy, float ex, float ey );
	~Collider();

	void render() const;

private:

	Quad *quad;

	vector< Collide > newGroup;
	vector< vector< Collide > * > collides;

	vector< bool > active;
	vector< int > cleared;

	Collider( const Collider &x ); // do not implement
	void operator=( const Collider &x ); // see above

	bool quadTest( const Float4 &line, const Float4 &range, const Quad *node ) const;
	void quadAdd( const Float4 &range, Collide *ptr, Quad *node );
	void quadRemove( const Float4 &range, Collide *ptr, Quad *node ); 

};

#endif
