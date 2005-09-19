#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
#include <map>
#include <set>
using namespace std;

#include "util.h"
#include "coord.h"

#define ENABLE_COLLIDE_DEBUG_VIS

class Collider {
public:

	void reset( int players );

    void startToken( int toki );
	void token( const Coord4 &line, const Coord4 &direction );

    void clearGroup( int category, int gid );

	void removeThingsFromGroup( int category, int gid );
	void endRemoveThingsFromGroup();

	void addThingsToGroup( int category, int gid, bool log = false);
	void endAddThingsToGroup();

	bool doProcess();
	Coord getCurrentTimestamp() const;
    void setCurrentTimestamp( Coord t );
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
    bool log;

	Coord ctime;

	pair< pair< int, int >, int > lhs;
	pair< pair< int, int >, int > rhs;

	vector< vector< pair< int, pair< Coord4, Coord4 > > > > items;
    int curpush;
    int curtoken;

    int players;

    set< int > moved;

    bool canCollide( int indexa, int indexb ) const;
    int getIndex( int category, int gid ) const;
    pair< int, int > reverseIndex( int index ) const;

};

#endif
