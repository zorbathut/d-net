#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
#include <map>
#include <set>
using namespace std;

#include "util.h"

#define ENABLE_COLLIDE_DEBUG_VIS

/*
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
	bool node;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	float r, g, b;
#endif
	Quad();
	Quad( const Float4 &dim );
	~Quad();
};
*/

class Collider {
public:

	void reset( int players );

    void startToken( int toki );
	void token( const Float4 &line, const Float4 &direction );

    void clearGroup( int category, int gid );

	void removeThingsFromGroup( int category, int gid );
	void endRemoveThingsFromGroup();

	void addThingsToGroup( int category, int gid );
	void endAddThingsToGroup();

	bool doProcess();
	float getCurrentTimestamp() const;
    void setCurrentTimestamp( float t );
	pair< pair< int, int >, int > getLhs() const;
	pair< pair< int, int >, int > getRhs() const;
  
    void flagAsMoved( int category, int gid );
    // flags to run a series of tests to make sure it's not *currently* impacting anything, and spit out collision errors if so
    // currently will also error and crash if it's colliding with wall or tank

	Collider();	// doesn't init to a usable state
	//void reinit( float sx, float sy, float ex, float ey );
	//Collider( float sx, float sy, float ex, float ey );
	~Collider();

	void render() const;
    
    bool testCollideSingle(int lhs, int rhs, bool print = false) const;
    bool testCollideAgainst(int active) const;
    bool testCollideAll(bool print = false) const;

private:
	
	int state;

	float ctime;

	pair< pair< int, int >, int > lhs;
	pair< pair< int, int >, int > rhs;

	vector< vector< pair< int, pair< Float4, Float4 > > > > items;
    int curpush;
    int curtoken;

    int players;

    set< int > moved;

    bool canCollide( int indexa, int indexb ) const;
    int getIndex( int category, int gid ) const;
    pair< int, int > reverseIndex( int index ) const;

	/*
	Quad *quad;

	vector< Collide > newGroup;
	vector< vector< Collide > * > collides;

	vector< bool > active;
	vector< int > cleared;

	Collider( const Collider &x ); // do not implement
	void operator=( const Collider &x ); // see above

	bool quadTest( const Float4 &line, const Float4 &range, const Quad *node ) const;
	pair< float, int > quadImpact( const Float4 &line, const Float4 &range, const Quad *node ) const;
	void quadAdd( const Float4 &range, Collide *ptr, Quad *node );
	void quadRemove( const Float4 &range, Collide *ptr, Quad *node ); */

};

#endif
