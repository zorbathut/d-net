
#include "game.h"

#include <cmath>
#include <vector>
#include <assert.h>
using namespace std;

#include <GL/gl.h>

#include "debug.h"
#include "gfx.h"
#include "const.h"
#include "collide.h"

void Tank::setPos( float in_x, float in_y ) {
	x = in_x;
	y = in_y;
}

const float tankvel = 24.f / FPS / SUBSTEP;
const float tankturn = 2.f / FPS / SUBSTEP;

void Tank::move( const Keystates &keystates, int phase ) {
	if( phase == 0 ) {
		int dv = keystates.forward - keystates.back;
		x += tankvel * dv * sin( d );
		y += -tankvel * dv * cos( d );
	} else if( phase == 1 ) {
		int dd = keystates.left - keystates.right;
		d += tankturn * dd;
		d += 2*PI;
		d = fmod( d, 2*(float)PI );
	} else {
		assert( 0 );
	}
};

void Tank::tick() { };

void Tank::render() const {
	glColor3f( 0.8f, 0.8f, 0.8f );
	drawLinePath( getTankVertices(), 1 );
};

bool Tank::colliding( const Collider &collider ) const {
	vector< float > tankpts = getTankVertices();
	for( int i = 0; i < 3; i++ )
		if( collider.test( tankpts[ i*2 ], tankpts[ i*2 + 1 ], tankpts[ ( i*2 + 2 ) % 6 ], tankpts[ ( i*2 + 3 ) % 6 ] ) )
			return true;
	return false;
};

const float tank_width = 5;
const float tank_length = tank_width*1.3;

float tank_coords[3][2] =  {
	{-tank_width / 2, -tank_length / 3},
	{ tank_width / 2, -tank_length / 3},
	{ 0, tank_length * 2 / 3 }
};

vector< float > Tank::getTankVertices() const {
	float xtx = cos( d );
	float xty = sin( d );
	float ytx = sin( d );
	float yty = -cos( d );
	vector< float > rv;
	for( int i = 0; i < 3; i++ ) {
		rv.push_back( x + tank_coords[ i ][ 0 ] * xtx + tank_coords[ i ][ 1 ] * xty );
		rv.push_back( y + tank_coords[ i ][ 1 ] * yty + tank_coords[ i ][ 0 ] * ytx );
	}
	return rv;
};

Tank::Tank() {
	x = 0;
	y = 0;
	d = 0;
}

void Gamemap::render() const {
	drawLine( 5, 5, 120, 5, 3 );
	drawLine( 5, 95, 120, 95, 3 );
	drawLine( 5, 95, 5, 5, 3 );
	drawLine( 120, 95, 120, 5, 3 );
}
void Gamemap::addCollide( Collider *collider ) const {
	collider->add( 5, 5, 120, 5 );
	collider->add( 5, 95, 120, 95 );
	collider->add( 5, 95, 5, 5 );
	collider->add( 120, 95, 120, 5 );
}

void Game::renderToScreen( int target ) const {
	player.render();
	gamemap.render();
};
void Game::runTick( const vector< Keystates > &keys ) {

	frameNm++;
	assert( keys.size() == 1 );

	Collider collider;
	gamemap.addCollide( &collider );

	for( int i = 0; i < SUBSTEP; i++ ) {
		assert( !player.colliding( collider ) );
		Tank playerbackup;
		playerbackup = player;
		playerbackup.move( keys[ 0 ], 0 );
		if( !playerbackup.colliding( collider ) )
			player = playerbackup;
		playerbackup = player;
		playerbackup.move( keys[ 0 ], 1 );
		if( !playerbackup.colliding( collider ) )
			player = playerbackup;
		player.tick();
		assert( !player.colliding( collider ) );
	}

};

Game::Game() {
	frameNm = 0;
	player.setPos( 30.f, 30.f );
};


