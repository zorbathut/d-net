#ifndef DNET_GAME
#define DNET_GAME

#include <vector>
using namespace std;

class Keystates {
public:
};

#define RENDERTARGET_SPECTATOR -1

class Game {
public:

	void renderToScreen( int player ) const;
	void runTick( const vector< Keystates > &keys );

	Game();

private:

	int frameNm;

};

#endif
