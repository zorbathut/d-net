
#include "game.h"

#include "debug.h"

void Game::renderToScreen( int player ) const { };
void Game::runTick( const vector< Keystates > &keys ) {
	dprintf( "Running frame %d\n", frameNm );
	frameNm++;
};

Game::Game() {
	frameNm = 0;
};
