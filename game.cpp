
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

const float tankvel = 24.f / FPS;
const float tankturn = 2.f / FPS;

void GfxEffects::move() {
	age++;
}
void GfxEffects::render() const {
    float apercent = 1.0f - (float)age / life;
	setColor( apercent, apercent, apercent );
    if( type == EFFECT_LINE ) {
        drawLine( pos + vel * age, 0.1f );
    } else if( type == EFFECT_POINT ) {
        drawPoint( pos.sx + vel.sx * age, pos.sy + vel.sy * age, 0.1f );
    } else {
        CHECK(0);
    }
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
		CHECK( 0 );
	}

	drawLinePath( getTankVertices( x, y, d ), 0.2, true );
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
    
	int dv = keys.u.down - keys.d.down;
	x += tankvel * dv * fsin( d ) * t;
	y += -tankvel * dv * fcos( d ) * t;

	int dd = keys.r.down - keys.l.down;
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
            ngfe.type = GfxEffects::EFFECT_LINE;
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
    CHECK( timeLeft == 1.0 );
	collider->token( Float4( x, y, x + projectile_length * fsin( d ), y - projectile_length * fcos( d ) ), Float4( projectile_speed * fsin( d ), -projectile_speed * fcos( d ), projectile_speed * fsin( d ), -projectile_speed * fcos( d ) ) );
};
void Projectile::impact( Tank *target ) {
	target->takeDamage( 1 );
};

void Projectile::genEffects( vector< GfxEffects > *gfxe ) const {
    GfxEffects ngfe;
    ngfe.pos.sx = x + projectile_length * fsin( d );
    ngfe.pos.sy = y - projectile_length * fcos( d );
    ngfe.life = 10;
    ngfe.type = GfxEffects::EFFECT_POINT;
    for( int i = 0; i < 3; i++ ) {
        float dir = frand() * 2 * PI;
        ngfe.vel.sx = fsin( dir );
        ngfe.vel.sy = -fcos( dir );
        ngfe.vel /= 5;
        ngfe.vel *= 1.0 - frand() * frand();
        gfxe->push_back( ngfe );
    }
}

Projectile::Projectile() {
    live = true;
}

void Game::renderToScreen( int target ) const {
    setZoom( 0, 0, 100 );
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

void collideHandler( Collider *collider, vector< Tank > *tanks, const vector< Keystates > &keys, const vector< int > &tankx ) {
    
    vector< Keystates > stopped = keys;
    for( int i = 0; i < stopped.size(); i++ ) {
        stopped[ i ].u.down = false;
        stopped[ i ].d.down = false;
        stopped[ i ].l.down = false;
        stopped[ i ].r.down = false;
    }
    
    Tank temptank;
    
    float cCollideTimeStamp = collider->getCurrentTimestamp() + COLLISIONROLLBACK;
    
    float cTimeStamp = collider->getCurrentTimestamp();
    float rollbackStep = COLLISIONROLLBACK;
    
    {
        collider->setCurrentTimestamp( cCollideTimeStamp );
        CHECK( collider->testCollideAll() );
        collider->setCurrentTimestamp( cTimeStamp );
    }
    
    //dprintf( "Original timestamp is %f\n", cCollideTimeStamp );
    
    while(1) {
        
        // If this triggers, we've gotten ourselves in a weird but potentially possible situation
        // The solution is to figure out *all* the tanks involved in the deadlock, and keep them all from moving
        // And possibly flash a "treads locked" message so they can figure out how to get out :P
        CHECK( cTimeStamp > 0.0 );
        
        cTimeStamp = max( cTimeStamp - rollbackStep, 0.0f );
        collider->setCurrentTimestamp( cTimeStamp );
        rollbackStep += rollbackStep;
        for( int i = 0; i < tankx.size(); i++ ) {
            temptank = (*tanks)[tankx[i]];
            temptank.move( cTimeStamp );
            temptank.setKeys( stopped[ i ] );
            collider->clearGroup(0, tankx[i]);
            collider->addThingsToGroup(0, tankx[i]);
            collider->startToken(0);
            temptank.addCollision( collider );
            collider->endAddThingsToGroup();
        }
        bool collided = false;
        for( int i = 0; i < tankx.size() && !collided; i++ )
            if( collider->testCollideAgainst( tankx[ i ] ) )
                collided = true;
        if( !collided )
            break;
        //dprintf( "Multiple rollback! %f\n", rollbackStep );
        // roll back again
    }
    
    //dprintf( "Unfreeze timestamp is %f\n", cTimeStamp );
    
    /* now we have our real timestamp, so let's see what can be released */
    /* all the tanks are still in the "fixed" location", so let's roll forward to the collide point and see what it looks like then */
    
    collider->setCurrentTimestamp( cCollideTimeStamp );
    
    vector< char > mustBeFrozen( tankx.size() );
    
    for( int i = 0; i < tankx.size(); i++ ) {
        //dprintf( "Retesting %d\n", i );
        // rewind, place tank in again
        collider->setCurrentTimestamp( 0 );
        temptank = (*tanks)[tankx[i]];
        collider->clearGroup(0, tankx[i]);
        collider->addThingsToGroup(0, tankx[i]);
        collider->startToken(0);
        temptank.addCollision( collider );
        collider->endAddThingsToGroup();
        collider->setCurrentTimestamp( cCollideTimeStamp );
        
        //dprintf( "Clix\n" );
        // if it's colliding again, it clearly can't move forward
        if( collider->testCollideAgainst( tankx[ i ] ) )
            mustBeFrozen[ i ] = true;
        
        //dprintf( "Reset\n" );
        
        temptank = (*tanks)[tankx[i]];
        temptank.move( cTimeStamp );
        temptank.setKeys( stopped[ i ] );
        collider->clearGroup(0, tankx[i]);
        collider->addThingsToGroup(0, tankx[i]);
        collider->startToken(0);
        temptank.addCollision( collider );
        collider->endAddThingsToGroup();
        //dprintf( "Done\n" );
    }
    
    //for( int i = 0; i < tankx.size(); i++ )
        //dprintf( "Freeze tokens: %d is %d\n", tankx[ i ], (int)mustBeFrozen[ i ] );
    
    if( count( mustBeFrozen.begin(), mustBeFrozen.end(), false ) == mustBeFrozen.size() ) {
        dprintf( "Mutual dependency, freezing all" );
        mustBeFrozen.clear();
        mustBeFrozen.resize( tankx.size(), true );
    }
    
    collider->setCurrentTimestamp( 0 );
    
    /* the frozen ones are still frozen at the appropriate time, so let's unfreeze the non-frozen ones, rollback to our "safe" time, and let the sim keep going from there */
    
    for( int i = 0; i < tankx.size(); i++ ) {
        if( mustBeFrozen[ i ] ) {
            (*tanks)[tankx[i]].move( cTimeStamp );
            (*tanks)[tankx[i]].setKeys( stopped[ i ] );
            collider->flagAsMoved( 0, tankx[i] );
        } else {
            temptank = (*tanks)[tankx[i]];
            collider->clearGroup(0, tankx[i]);
            collider->addThingsToGroup(0, tankx[i]);
            collider->startToken(0);
            temptank.addCollision( collider );
            collider->endAddThingsToGroup();
        }            
    }
    
    collider->setCurrentTimestamp( cTimeStamp );
    
    //dprintf( "Sim continuing\n" );

    CHECK( !collider->testCollideAll() );
    
}

bool Game::runTick( const vector< Keystates > &keys ) {
    
	frameNm++;
	CHECK( keys.size() == 2 );

	collider.reset(players.size());
    
	collider.addThingsToGroup(-1, 0);
    collider.startToken(0);
	gamemap.addCollide( &collider );
	collider.endAddThingsToGroup();
    
	for( int k = 0; k < players.size(); k++ ) {
		collider.addThingsToGroup(0, k);
        collider.startToken(0);
        players[ k ].startNewMoveCycle();
        players[ k ].setKeys( keys[ k ] );
		players[ k ].addCollision( &collider );
		collider.endAddThingsToGroup();
	}
    
	for( int j = 0; j < projectiles.size(); j++ ) {
		collider.addThingsToGroup(1, j);
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            collider.startToken(k);
            projectiles[ j ][ k ].startNewMoveCycle();
			projectiles[ j ][ k ].addCollision( &collider );
		}
		collider.endAddThingsToGroup();
	}
    
    CHECK( !collider.testCollideAll() );
    
	while( collider.doProcess() ) {
		//dprintf( "Collision!\n" );
		//dprintf( "Timestamp %f\n", collider.getCurrentTimestamp() );
		//dprintf( "%d,%d,%d vs %d,%d,%d\n", collider.getLhs().first.first, collider.getLhs().first.second, collider.getLhs().second, collider.getRhs().first.first, collider.getRhs().first.second, collider.getRhs().second );
        pair< pair< int, int >, int > lhs = collider.getLhs();
        pair< pair< int, int >, int > rhs = collider.getRhs();
        if( lhs > rhs ) swap( lhs, rhs );
        if( lhs.first.first == -1 && rhs.first.first == -1 ) {
            // wall-wall collision, wtf?
            CHECK(0);
        } else if( lhs.first.first == -1 && rhs.first.first == 0 ) {
            
            CHECK(rhs.first.second >= 0 && rhs.first.second < players.size());
            
            // wall-tank collision - stop tank
            //dprintf( "Wall-tank collision\n" );
            //dprintf( "Time currently %f\n", collider.getCurrentTimestamp() );
            
            vector< int > tankcollide;
            tankcollide.push_back(rhs.first.second);
            collideHandler( &collider, &players, keys, tankcollide );
            
        } else if( lhs.first.first == -1 && rhs.first.first == 1 ) {
            // wall-projectile collision - kill projectile
            collider.removeThingsFromGroup( rhs.first.first, rhs.first.second );
            collider.startToken(rhs.second);
            projectiles[ rhs.first.second ][ rhs.second ].addCollision( &collider );
            collider.endRemoveThingsFromGroup();
            projectiles[ rhs.first.second ][ rhs.second ].live = false;
            projectiles[ rhs.first.second ][ rhs.second ].genEffects( &gfxeffects );
        } else if( lhs.first.first == 0 && rhs.first.first == 0 ) {
            // tank-tank collision
            //dprintf( "Tank-tank collision\n" );
            //dprintf( "Time currently %f\n", collider.getCurrentTimestamp() );
            
            vector< int > tankcollide;
            tankcollide.push_back(lhs.first.second);
            tankcollide.push_back(rhs.first.second);
            collideHandler( &collider, &players, keys, tankcollide );

        } else if( lhs.first.first == 0 && rhs.first.first == 1 ) {
            // tank-projectile collision - kill projectile, do damage
            collider.removeThingsFromGroup( rhs.first.first, rhs.first.second );
            collider.startToken(rhs.second);
            projectiles[ rhs.first.second ][ rhs.second ].addCollision( &collider );
            collider.endRemoveThingsFromGroup();
            projectiles[ rhs.first.second ][ rhs.second ].impact( &players[ lhs.first.second ] );
            projectiles[ rhs.first.second ][ rhs.second ].live = false;
            projectiles[ rhs.first.second ][ rhs.second ].genEffects( &gfxeffects );
        } else if( lhs.first.first == 1 && rhs.first.first == 1 ) {
            // projectile-projectile collision - kill both projectiles
            collider.removeThingsFromGroup( lhs.first.first, lhs.first.second );
            collider.startToken(lhs.second);
            projectiles[ lhs.first.second ][ lhs.second ].addCollision( &collider );
            collider.endRemoveThingsFromGroup();
            projectiles[ lhs.first.second ][ lhs.second ].live = false;
            projectiles[ lhs.first.second ][ lhs.second ].genEffects( &gfxeffects );
            
            collider.removeThingsFromGroup( rhs.first.first, rhs.first.second );
            collider.startToken(rhs.second);
            projectiles[ rhs.first.second ][ rhs.second ].addCollision( &collider );
            collider.endRemoveThingsFromGroup();
            projectiles[ rhs.first.second ][ rhs.second ].live = false;
            projectiles[ rhs.first.second ][ rhs.second ].genEffects( &gfxeffects );
        } else {
            dprintf("omgwtf %d %d\n", lhs.first.first, rhs.first.first);
            CHECK(0);
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
		if( players[ i ].live && keys[ i ].f.repeat ) {
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
    
    {
        int playersleft = 0;
        for(int i = 0; i < players.size(); i++) {
            if(players[i].live)
                playersleft++;
        }
        if(playersleft <= 1) {
            framesSinceOneLeft++;
        }
    }
    
    return framesSinceOneLeft / FPS >= 3;

};

Game::Game() /*: collider( 0, 0, 125, 100 ) */ {
	frameNm = 0;
	players.resize( 2 );
	players[ 0 ].x = 30;
	players[ 0 ].y = 30;
	players[ 1 ].x = 60;
	players[ 1 ].y = 60;
	projectiles.resize( 2 );
    framesSinceOneLeft = 0;
	//collider.startGroup();
	//gamemap.addCollide( &collider );
	//collider.endGroup();
};



