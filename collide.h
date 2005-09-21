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

	void addThingsToGroup( int category, int gid, bool log = false);
	void endAddThingsToGroup();

	void process();

    bool next();
	pair< pair< int, int >, int > getLhs() const;
	pair< pair< int, int >, int > getRhs() const;

	Collider();
	~Collider();

	void render() const;

private:
	
    enum { CSTA_WAIT, CSTA_ADD, CSTA_PROCESSED };
    
	int state;
    bool log;

    int curcollide;
    vector< pair< pair< pair< int, int >, int >, pair< pair< int, int >, int > > > collides;

	vector< vector< pair< int, pair< Coord4, Coord4 > > > > items;
    int curpush;
    int curtoken;

    int players;

    bool canCollide( int indexa, int indexb ) const;
    int getIndex( int category, int gid ) const;
    pair< int, int > reverseIndex( int index ) const;

};

#endif
