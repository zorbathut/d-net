
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

Keystates::Keystates() {
	memset( this, 0, sizeof( *this ) );
};

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

void Tank::render( int tankid ) const {
	if( tankid == 0 ) {
		glColor3f( 0.8f, 0.0f, 0.0f );
	} else if( tankid == 1 ) {
		glColor3f( 0.0f, 0.8f, 0.0f );
	} else {
		assert( 0 );
	}

	drawLinePath( getTankVertices(), 0.2 );
};

bool Tank::colliding( const Collider &collider ) const {
	vector< float > tankpts = getTankVertices();
	for( int i = 0; i < 3; i++ )
		if( collider.test( tankpts[ i*2 ], tankpts[ i*2 + 1 ], tankpts[ ( i*2 + 2 ) % 6 ], tankpts[ ( i*2 + 3 ) % 6 ] ) )
			return true;
	return false;
};

void Tank::addCollision( Collider *collider ) const {
	vector< float > tankpts = getTankVertices();
	for( int i = 0; i < 3; i++ )
		collider->add( tankpts[ i*2 ], tankpts[ i*2 + 1 ], tankpts[ ( i*2 + 2 ) % 6 ], tankpts[ ( i*2 + 3 ) % 6 ] );
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
	glColor3f( 0.5f, 0.5f, 0.5f );
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
	for( int i = 0; i < players.size(); i++ ) {
		players[ i ].render( i );
	};
	gamemap.render();
};
void Game::runTick( const vector< Keystates > &keys ) {

	frameNm++;
	assert( keys.size() == 2 );

	Collider collider;
	collider.startGroup();
	gamemap.addCollide( &collider );
	collider.endGroup();

	for( int i = 0; i < SUBSTEP; i++ ) {

		for( int j = 0; j < PHASECOUNT; j++ ) {

			vector< int > tcid;

			// first, add all their collision points
			for( int k = 0; k < players.size(); k++ ) {
				collider.startGroup();
				assert( !players[ k ].colliding( collider ) );
				players[ k ].addCollision( &collider );
				tcid.push_back( collider.endGroup() );
			}

			vector< Tank > newtanks;
			// next, try moving them, see if it works
			for( int k = 0; k < players.size(); k++ ) {
				collider.disableGroup( tcid[ k ] );
				Tank testtank = players[ k ];
				testtank.move( keys[ k ], j );
				if( testtank.colliding( collider ) )
					newtanks.push_back( players[ k ] );
				else
					newtanks.push_back( testtank );
				collider.enableGroup( tcid[ k ] );
			}
			
			// remove the old collision points, add the new ones
			for( int k = 0; k < players.size(); k++ ) {
				collider.disableGroup( tcid[ k ] );
				collider.startGroup();
				newtanks[ k ].addCollision( &collider );
				tcid[ k ] = collider.endGroup();
			}

			// now look for conflicts
			for( int k = 0; k < players.size(); k++ ) {
				collider.disableGroup( tcid[ k ] );
				if( !newtanks[ k ].colliding( collider ) )
					players[ k ] = newtanks[ k ];
				collider.enableGroup( tcid[ k ] );
			}

			// now clear all the dynamic collision points
			for( int k = 0; k < players.size(); k++ ) {
				collider.disableGroup( tcid[ k ] );
			}

			// in theory I could now add their real location and check to make sure they're not colliding still

		}

	}

	for( int i = 0; i < players.size(); i++ )
		players[ i ].tick();

};

Game::Game() {
	frameNm = 0;
	players.resize( 2 );
	players[ 0 ].setPos( 30.f, 30.f );
	players[ 1 ].setPos( 60.f, 60.f );

};


