
#include "game.h"

#include <cmath>
#include <vector>
#include <assert.h>
using namespace std;

#include <GL/gl.h>

#include "debug.h"
#include "gfx.h"
#include "const.h"

void Tank::setPos( float in_x, float in_y ) {
	x = in_x;
	y = in_y;
}

const float tankvel = 24.f / FPS;
const float tankturn = 2.f / FPS;

void Tank::move( const Keystates &keystates ) {
	int dv = keystates.forward - keystates.back;
	int dd = keystates.left - keystates.right;
	dprintf( "4: %d, %d, %d, %d -> %d, %d :: %d\n", keystates.forward, keystates.back, keystates.left, keystates.right, dv, dd, sizeof( vector< Keystates > ) );
	x += tankvel * dv * sin( d );
	y += -tankvel * dv * cos( d );
	d += tankturn * dd;
	d += 2*PI;
	d = fmod( d, 2*(float)PI );
};

void Tank::render() const {
	drawTank( x, y, d );
};

Tank::Tank() {
	x = 0;
	y = 0;
	d = 0;
}

void Game::renderToScreen( int target ) const {
	player.render();
};
void Game::runTick( const vector< Keystates > &keys ) {
	frameNm++;
	assert( keys.size() == 1 );
	player.move( keys[ 0 ] );
};

Game::Game() {
	frameNm = 0;
	player.setPos( 30.f, 30.f );
};
