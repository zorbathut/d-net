#ifndef DNET_GAME
#define DNET_GAME

#include <vector>
using namespace std;

class Collider;

class Keystates {
public:
	char forward, back, left, right;
};

#define RENDERTARGET_SPECTATOR -1

class Tank {
public:
	void setPos( float x, float y );
	void move( const Keystates &keystates, int phase );
	void tick();
	void render() const;

	bool colliding( const Collider &collider ) const;
	Tank();

	vector< float > getTankVertices() const;
	
private:
	float x;
	float y;
	float d;
};

class Gamemap {
public:

	void render() const;
	void addCollide( Collider *collide ) const;

	Gamemap();

private:

	vector< float > vertices;


};

class Game {
public:

	void renderToScreen( int player ) const;
	void runTick( const vector< Keystates > &keys );

	Game();

private:

	int frameNm;

	Tank player;
	Gamemap gamemap;

};

#endif
