#ifndef DNET_GAME
#define DNET_GAME

#include <vector>
using namespace std;

class Keystates {
public:
	char forward, back, left, right;
};

#define RENDERTARGET_SPECTATOR -1

class Tank {
public:
	void setPos( float x, float y );
	void move( const Keystates &keystates );
	void render() const;
	Tank();
	
private:
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

	Tank player;

};

#endif
