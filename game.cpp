
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
	drawLinePath( getTankVertices(), 0.2 );
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
	drawLinePath( vertices, 0.5 );
}
void Gamemap::addCollide( Collider *collider ) const {
	for( int i = 0; i < vertices.size(); i += 2 )
		collider->add( vertices[ i ], vertices[ i + 1 ], vertices[ ( i + 2 ) % vertices.size() ], vertices[ ( i + 3 ) % vertices.size() ] );
}

Gamemap::Gamemap() {

	// bl corner
	vertices.push_back( 5 );
	vertices.push_back( 5 );

	// bottom bump
	{
		vertices.push_back( 40 );
		vertices.push_back( 5 );

		vertices.push_back( 50 );
		vertices.push_back( 24 );

		vertices.push_back( 70 );
		vertices.push_back( 19 );

		vertices.push_back( 80 );
		vertices.push_back( 5 );
	}

	// br corner
	vertices.push_back( 120 );
	vertices.push_back( 5 );

	// right wedge
	{
		vertices.push_back( 120 );
		vertices.push_back( 15 );

		vertices.push_back( 110 );
		vertices.push_back( 15 );

		vertices.push_back( 120 );
		vertices.push_back( 40 );
	}

	// tr corner
	{
		vertices.push_back( 120 );
		vertices.push_back( 80 );

		vertices.push_back( 115 );
		vertices.push_back( 95 );
	}

	// top depression
	{
		vertices.push_back( 90 );
		vertices.push_back( 95 );

		vertices.push_back( 70 );
		vertices.push_back( 85 );

		vertices.push_back( 55 );
		vertices.push_back( 88 );

		vertices.push_back( 45 );
		vertices.push_back( 97 );

		vertices.push_back( 30 );
		vertices.push_back( 95 );
	}

	// tl corner
	{
		vertices.push_back( 5 );
		vertices.push_back( 95 );
	}

	// left bumpiness
	{
		vertices.push_back( 5 );
		vertices.push_back( 80 );

		vertices.push_back( 20 );
		vertices.push_back( 74 );

		vertices.push_back( 35 );
		vertices.push_back( 60 );

		vertices.push_back( 38 );
		vertices.push_back( 40 );

		vertices.push_back( 33 );
		vertices.push_back( 58 );

		vertices.push_back( 17 );
		vertices.push_back( 70 );

		vertices.push_back( 10 );
		vertices.push_back( 70 );

		vertices.push_back( 5 );
		vertices.push_back( 62 );
	}
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


