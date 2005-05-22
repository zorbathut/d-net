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
	bool node;
#ifdef ENABLE_COLLIDE_DEBUG_VIS
	float r, g, b;
#endif
	Quad();
	Quad( const Float4 &dim );
	~Quad();
};

class Collider {
public:

	void reset();

    void startToken( int toki );
	void token( const Float4 &line, const Float4 &direction );

	void createGroup();
	int endCreateGroup();

    void clearGroup( int gid );

	void removeThingsFromGroup( int gid );
	void endRemoveThingsFromGroup();

	void addThingsToGroup( int gid );
	void endAddThingsToGroup();

	bool doProcess();
	float getCurrentTimestamp();
    void setCurrentTimestamp( float t );
	pair< int, int > getLhs();
	pair< int, int > getRhs();
  
    void flagAsMoved( int group );

	Collider();	// doesn't init to a usable state
	//void reinit( float sx, float sy, float ex, float ey );
	//Collider( float sx, float sy, float ex, float ey );
	~Collider();

	void render() const;
    
    bool testCollideAgainst( int active, int start, int end, float time );

private:
	
	int state;

	float ctime;

	pair< int, int > lhs;
	pair< int, int > rhs;

	vector< vector< pair< int, pair< Float4, Float4 > > > > items;
    int curpush;
    int curtoken;

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
