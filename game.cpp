
#include "game.h"

#include <cmath>
#include <vector>
#include <algorithm>
using namespace std;

#include <GL/gl.h>

#include "debug.h"
#include "gfx.h"
#include "const.h"
#include "collide.h"
#include "util.h"

Keystates::Keystates() {
	memset( this, 0, sizeof( *this ) );
};

const float tankvel = 24.f / FPS;
const float tankturn = 2.f / FPS;

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

	drawLinePath( getTankVertices( x, y, d ), 0.2 );
};

void Tank::startNewMoveCycle() {
    timeLeft = 1.0;
};

void Tank::setKeys( const Keystates &keystates ) {
    keys = keystates;
};

void Tank::move() {
    move( timeLeft );
}

void Tank::move( float time ) {
	if( !live )
		return;
    
    pair< pair< float, float >, float > newpos = getDeltaAfterMovement( keys, x, y, d, time );
    
    x = newpos.first.first;
    y = newpos.first.second;
    d = newpos.second;
    
    timeLeft -= time;
	
};

void Tank::addCollision( Collider *collider ) const {
    
	if( !live )
		return;

	vector< float > tankpts = getTankVertices( x, y, d );
    pair< pair< float, float >, float > newpos = getDeltaAfterMovement( keys, x, y, d, timeLeft );
	vector< float > newtankpts = getTankVertices( newpos.first.first, newpos.first.second, newpos.second );
	for( int i = 0; i < newtankpts.size(); i++ )
		newtankpts[ i ] -= tankpts[ i ];
	for( int i = 0; i < 3; i++ )
		collider->token( Float4( tankpts[ i*2 ], tankpts[ i*2 + 1 ], tankpts[ ( i*2 + 2 ) % 6 ], tankpts[ ( i*2 + 3 ) % 6 ] ), Float4( newtankpts[ i*2 ], newtankpts[ i*2 + 1 ], newtankpts[ ( i*2 + 2 ) % 6 ], newtankpts[ ( i*2 + 3 ) % 6 ] ) );
};

const float tank_width = 5;
const float tank_length = tank_width*1.3;

float tank_coords[3][2] =  {
	{-tank_width / 2, -tank_length / 3},
	{ tank_width / 2, -tank_length / 3},
	{ 0, tank_length * 2 / 3 }
};

vector< float > Tank::getTankVertices( float tx, float ty, float td ) const {
	float xtx = fcos( td );
	float xty = fsin( td );
	float ytx = fsin( td );
	float yty = -fcos( td );
	vector< float > rv;
	for( int i = 0; i < 3; i++ ) {
		rv.push_back( tx + tank_coords[ i ][ 0 ] * xtx + tank_coords[ i ][ 1 ] * xty );
		rv.push_back( ty + tank_coords[ i ][ 1 ] * yty + tank_coords[ i ][ 0 ] * ytx );
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

pair< pair< float, float >, float > Tank::getDeltaAfterMovement( const Keystates &keys, float x, float y, float d, float t ) const {
    
	int dv = keys.forward - keys.back;
	x += tankvel * dv * fsin( d ) * t;
	y += -tankvel * dv * fcos( d ) * t;

	int dd = keys.left - keys.right;
	d += tankturn * dd * t;
	d += 2*PI;
	d = fmod( d, 2*(float)PI );
    
    return make_pair( make_pair( x, y ), d );
    
}

void Tank::takeDamage( int damage ) {
	health -= damage;
	if( health <= 0 ) {
		live = false;
		spawnShards = true;
	}
};

void Tank::genEffects( vector< GfxEffects > *gfxe ) {
	if( spawnShards ) {
		vector< float > tv = getTankVertices( x, y, d );
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
const float projectile_speed = 60.f / FPS;

void Projectile::startNewMoveCycle() {
    timeLeft = 1.0f;
}
void Projectile::move() {
    move( timeLeft );
}
void Projectile::move( float time ) {
    x += projectile_speed * fsin( d ) * time;
    y += -projectile_speed * fcos( d ) * time;
    timeLeft -= time;
}

void Projectile::render() const {
	setColor( 1.0, 1.0, 1.0 );
	drawLine( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ), 0.1 );
};
void Projectile::addCollision( Collider *collider ) const {
    assert( timeLeft == 1.0 );
	collider->token( Float4( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ) ), Float4( projectile_speed * fsin( d ), -projectile_speed * fcos( d ), projectile_speed * fsin( d ), -projectile_speed * fcos( d ) ) );
};
void Projectile::impact( Tank *target ) {
	target->takeDamage( 1 );
};

Projectile::Projectile() {
    live = true;
}

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

	collider.reset();
    
    dprintf("gm\n");

	collider.createGroup();
    collider.startToken(0);
	gamemap.addCollide( &collider );
	collider.endCreateGroup();
    
    dprintf("play\n");

	for( int k = 0; k < players.size(); k++ ) {
		collider.createGroup();
        collider.startToken(0);
        players[ k ].startNewMoveCycle();
        players[ k ].setKeys( keys[ k ] );
		players[ k ].addCollision( &collider );
		collider.endCreateGroup();
	}
    
    dprintf("proj\n");

	for( int j = 0; j < projectiles.size(); j++ ) {
		collider.createGroup();
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            collider.startToken(k);
            projectiles[ j ][ k ].startNewMoveCycle();
			projectiles[ j ][ k ].addCollision( &collider );
		}
		collider.endCreateGroup();
	}
    
    dprintf("done\n");
    
    for( int k = 0; k < players.size() + 1; k++ ) {
        assert( !collider.testCollideAgainst( k, k + 1, players.size() + 1, 0 ) );
    }
    
	while( collider.doProcess() ) {
		dprintf( "Collision!\n" );
		dprintf( "Timestamp %f\n", collider.getCurrentTimestamp() );
		dprintf( "%d,%d vs %d,%d\n", collider.getLhs().first, collider.getLhs().second, collider.getRhs().first, collider.getRhs().second );
        pair< int, int > lhs = collider.getLhs();
        pair< int, int > rhs = collider.getRhs();
        if( lhs > rhs ) swap( lhs, rhs );
        if( lhs.first == 0 && rhs.first == 0 ) {
            // wall-wall collision, wtf?
            assert(0);
        } else if( lhs.first == 0 && rhs.first <= players.size() ) {
            
            // wall-tank collision - stop tank
            dprintf( "Wall-tank collision\n" );
            dprintf( "Time currently %f\n", collider.getCurrentTimestamp() );
            
            int playerTarget = rhs.first - 1;
    
            Keystates stopped = keys[ playerTarget ];
            stopped.left = false;
            stopped.right = false;
            stopped.forward = false;
            stopped.back = false;
            
            Tank temptank;
            
            float cTimeStamp = collider.getCurrentTimestamp();
            float rollbackStep = COLLISIONROLLBACK;
            
            while(1) {
                assert( cTimeStamp > 0.0 );
                cTimeStamp = max( cTimeStamp - rollbackStep, 0.0f );
                rollbackStep += rollbackStep;
                temptank = players[ playerTarget ];
                temptank.move( cTimeStamp );
                temptank.setKeys( stopped );
                collider.clearGroup( rhs.first );
                collider.addThingsToGroup( rhs.first );
                collider.startToken(0);
                temptank.addCollision( &collider );
                collider.endAddThingsToGroup();
                if( !collider.testCollideAgainst( rhs.first, 0, players.size() + 1, collider.getCurrentTimestamp() ) )
                    break;
                dprintf( "Multiple rollback! %f\n", rollbackStep );
                // roll back again
            }
            
            players[ playerTarget ] = temptank;
            
            assert( !collider.testCollideAgainst( rhs.first, 0, players.size() + 1, collider.getCurrentTimestamp() ) );
            
            collider.flagAsMoved( rhs.first );
            
        } else if( lhs.first == 0 ) {
            // wall-projectile collision - kill projectile
            dprintf( "Wall-projectile collision\n" );
        } else if( lhs.first <= players.size() && rhs.first <= players.size() ) {
            // tank-tank collision
            dprintf( "Tank-tank collision\n" );
        } else if( lhs.first <= players.size() ) {
            // tank-projectile collision
            dprintf( "Tank-projectile collision\n" );
        } else {
            // projectile-projectile collision
            dprintf( "Projectile-projectile collision\n" );
        }
	}

	for( int j = 0; j < projectiles.size(); j++ ) {
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            if( !projectiles[ j ][ k ].live ) {
                projectiles[ j ].erase( projectiles[ j ].begin() + k );
                k--;
            } else {
            	projectiles[ j ][ k ].move();
            }
        }
    }

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
		players[ i ].move();

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

Game::Game() /*: collider( 0, 0, 125, 100 ) */ {
	frameNm = 0;
	players.resize( 2 );
	players[ 0 ].x = 30;
	players[ 0 ].y = 30;
	players[ 1 ].x = 60;
	players[ 1 ].y = 60;
	projectiles.resize( 2 );
	//collider.startGroup();
	//gamemap.addCollide( &collider );
	//collider.endGroup();
};


