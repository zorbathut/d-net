#ifndef DNET_GAME
#define DNET_GAME

#include "gamemap.h"

#include <vector>
using namespace std;

class Collider;

class Keystates {
public:
	char forward, back, left, right, firing;
	Keystates();
};

#define RENDERTARGET_SPECTATOR -1
#define PHASECOUNT 2

class Tank {
public:
	void move( const Keystates &keystates, int phase );
	void tick();
	void render( int tankid ) const;

	bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;
	Tank();

	vector< float > getTankVertices() const;
	pair< float, float > getFiringPoint() const;
	
	float x;
	float y;
	float d;
};

class Projectile {
public:

	void move();
	void render() const;
	bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;

	float x;
	float y;
	float d;

};

class Game {
public:

	void renderToScreen( int player ) const;
	void runTick( const vector< Keystates > &keys );

	Game();

private:

	int frameNm;

	vector< Tank > players;
	vector<vector< Projectile > > projectiles;
	Gamemap gamemap;

	Collider collider;

};

#endif
