
#include "game.h"

#include <cmath>
#include <vector>
#include <algorithm>
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

const float tankvel = 24.f / FPS / SUBSTEP;
const float tankturn = 2.f / FPS / SUBSTEP;

void GfxEffects::move() {
	age++;
}
void GfxEffects::render() const {
	float apercent = 1.0f - (float)age / life;
	setColor( apercent, apercent, apercent );
	drawLine( pos + vel * age, 0.1f );
}
bool GfxEffects::dead() const {
	return age >= life;
}


GfxEffects::GfxEffects() {
	age = 0;
}

void Tank::move( const Keystates &keystates, int phase ) {
	if( !live )
		return;

	if( phase == 0 ) {
		int dv = keystates.forward - keystates.back;
		x += tankvel * dv * fsin( d );
		y += -tankvel * dv * fcos( d );
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
	if( !live )
		return;

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
	if( !live )
		return false;
	vector< float > tankpts = getTankVertices();
	for( int i = 0; i < 3; i++ )
		if( collider.test( tankpts[ i*2 ], tankpts[ i*2 + 1 ], tankpts[ ( i*2 + 2 ) % 6 ], tankpts[ ( i*2 + 3 ) % 6 ] ) )
			return true;
	return false;
};

void Tank::addCollision( Collider *collider ) const {
	if( !live )
		return;
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
	float xtx = fcos( d );
	float xty = fsin( d );
	float ytx = fsin( d );
	float yty = -fcos( d );
	vector< float > rv;
	for( int i = 0; i < 3; i++ ) {
		rv.push_back( x + tank_coords[ i ][ 0 ] * xtx + tank_coords[ i ][ 1 ] * xty );
		rv.push_back( y + tank_coords[ i ][ 1 ] * yty + tank_coords[ i ][ 0 ] * ytx );
	}
	return rv;
};

pair< float, float > Tank::getFiringPoint() const {
	float xtx = fcos( d );
	float xty = fsin( d );
	float ytx = fsin( d );
	float yty = -fcos( d );
	return make_pair( x + tank_coords[ 2 ][ 0 ] * xtx + tank_coords[ 2 ][ 1 ] * xty, y + tank_coords[ 2 ][ 1 ] * yty + tank_coords[ 2 ][ 0 ] * ytx );
};

void Tank::takeDamage( int damage ) {
	health -= damage;
	if( health <= 0 ) {
		live = false;
		spawnShards = true;
	}
};

void Tank::genEffects( vector< GfxEffects > *gfxe ) {
	if( spawnShards ) {
		vector< float > tv = getTankVertices();
		for( int i = 0; i < tv.size(); i += 2 ) {
			GfxEffects ngfe;
			ngfe.pos.sx = tv[ i ];
			ngfe.pos.sy = tv[ i + 1 ];
			ngfe.pos.ex = tv[ ( i + 2 ) % tv.size() ];
			ngfe.pos.ey = tv[ ( i + 3 ) % tv.size() ];
			float cx = ( ngfe.pos.sx + ngfe.pos.ex ) / 2;
			float cy = ( ngfe.pos.sy + ngfe.pos.ey ) / 2;
			ngfe.vel.sx = ngfe.vel.ex = cx - x;
			ngfe.vel.sy = ngfe.vel.ey = cy - y;
			ngfe.vel /= 5;
			ngfe.life = 15;
			gfxe->push_back( ngfe );
		}
		spawnShards = false;
	}
}

Tank::Tank() {
	x = 0;
	y = 0;
	d = 0;
	live = true;
	spawnShards = false;
	health = 20;
}

const float projectile_length = 1;
const float projectile_speed = 60.f / FPS / SUBSTEP;

void Projectile::move() {
	x += projectile_speed * fsin( d );
	y += -projectile_speed * fcos( d );
};
void Projectile::render() const {
	setColor( 1.0, 1.0, 1.0 );
	drawLine( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ), 0.1 );
};
bool Projectile::colliding( const Collider &collider ) const {
	return collider.test( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ) );
};
pair< float, int > Projectile::getImpact( const Collider &collider ) const {
	return collider.getImpact( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ) );
};
void Projectile::addCollision( Collider *collider ) const {
	collider->add( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ) );
};
void Projectile::impact( Tank *target ) {
	target->takeDamage( 1 );
};

void Game::renderToScreen( int target ) const {
	for( int i = 0; i < players.size(); i++ )
		players[ i ].render( i );
	for( int i = 0; i < projectiles.size(); i++ )
		for( int j = 0; j < projectiles[ i ].size(); j++ )
			projectiles[ i ][ j ].render();
	for( int i = 0; i < gfxeffects.size(); i++ )
		gfxeffects[ i ].render();
	gamemap.render();
	collider.render();
};
void Game::runTick( const vector< Keystates > &keys ) {

	frameNm++;
	assert( keys.size() == 2 );

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
				collider.deleteGroup( tcid[ k ] );
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
			for( int k = 0; k < players.size(); k++ )
				collider.deleteGroup( tcid[ k ] );

			// in theory I could now add their real location and check to make sure they're not colliding still

		}

		for( int j = 0; j < projectiles.size(); j++ )
			for( int k = 0; k < projectiles[ j ].size(); k++ )
				projectiles[ j ][ k ].move();

		// add all the projectiles and update health, in preparation for the Final Movement
		if( i == SUBSTEP - 1 ) {

			for( int i = 0; i < players.size(); i++ ) {
				if( players[ i ].live && keys[ i ].firing ) {
					Projectile proj;
					proj.x = players[ i ].getFiringPoint().first;
					proj.y = players[ i ].getFiringPoint().second;
					proj.d = players[ i ].d;
					projectiles[ i ].push_back( proj );
				}
			}

			for( int i = 0; i < players.size(); i++ )
				players[ i ].tick();

		}

		vector< int > tcid;
		vector< int > pcid;
		for( int j = 0; j < players.size(); j++ ) {
			collider.startGroup();
			players[ j ].addCollision( &collider );
			tcid.push_back( collider.endGroup()  );
			collider.startGroup();
			for( int k = 0; k < projectiles[ j ].size(); k++ )
				projectiles[ j ][ k ].addCollision( &collider );
			pcid.push_back( collider.endGroup() );
		}
		vector< pair< Projectile, int > > impacts;
		vector< Projectile > liveproj;
		for( int j = 0; j < projectiles.size(); j++ ) {
			collider.disableGroup( tcid[ j ] );
			collider.disableGroup( pcid[ j ] );
			for( int k = 0; k < projectiles[ j ].size(); k++ ) {
				pair< float, int > impact = projectiles[ j ][ k ].getImpact( collider );
				if( impact.first == 2.0f ) {
					liveproj.push_back( projectiles[ j ][ k ] );
				} else {
					int target = find( tcid.begin(), tcid.end(), impact.second ) - tcid.begin();
					if( target < tcid.size() )
						impacts.push_back( make_pair( projectiles[ j ][ k ], target ) );
				}	
			}
			projectiles[ j ].swap( liveproj );
			liveproj.clear();
			collider.enableGroup( tcid[ j ] );
			collider.enableGroup( pcid[ j ] );
		}
		for( int j = 0; j < players.size(); j++ ) {
			collider.deleteGroup( tcid[ j ] );
			collider.deleteGroup( pcid[ j ] );
		}

		for( int i = 0; i < impacts.size(); i++ ) {
			dprintf( "Impact on player %d\n", impacts[ i ].second );
			impacts[ i ].first.impact( &players[ impacts[ i ].second ] );
		}

	}

	{
		vector< GfxEffects > neffects;
		for( int i = 0; i < gfxeffects.size(); i++ ) {
			gfxeffects[ i ].move();
			if( !gfxeffects[ i ].dead() )
				neffects.push_back( gfxeffects[ i ] );
		}
		swap( neffects, gfxeffects );
	}

	for( int i = 0; i < players.size(); i++ )
		players[ i ].genEffects( &gfxeffects );

};

Game::Game() : collider( 0, 0, 125, 100 ) {
	frameNm = 0;
	players.resize( 2 );
	players[ 0 ].x = 30;
	players[ 0 ].y = 30;
	players[ 1 ].x = 60;
	players[ 1 ].y = 60;
	projectiles.resize( 2 );
	collider.startGroup();
	gamemap.addCollide( &collider );
	collider.endGroup();
};


