
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
#include "args.h"
#include "rng.h"

DEFINE_bool(verboseCollisions, false, "Verbose collisions");

void Player::reCalculate() {
    maxHealth = 20;
    turnSpeed = 2.f / FPS;
    maxSpeed = Coord(24) / FPS;
    int healthMult = 100;
    int turnMult = 100;
    int speedMult = 100;
    for(int i = 0; i < upgrades.size(); i++) {
        healthMult += upgrades[i]->hull;
        turnMult += upgrades[i]->handling;
        speedMult += upgrades[i]->engine;
    }
    maxHealth *= healthMult;
    maxHealth /= 100;
    turnSpeed *= turnMult;
    turnSpeed /= 100;
    maxSpeed *= speedMult;
    maxSpeed /= 100;
}

bool Player::hasUpgrade(const Upgrade *upg) const {
    return count(upgrades.begin(), upgrades.end(), upg);
}

int Player::resellAmmoValue() const {
    return int(shotsLeft * weapon->costpershot * 0.8);
}

Player::Player() {
    color = Color(0.5, 0.5, 0.5);
    cash = 1000;
    reCalculate();
    weapon = defaultWeapon();
    shotsLeft = -1;
}

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

void Tank::init(Player *in_player) {
    CHECK(in_player);
    CHECK(!player);
    player = in_player;
    health = player->maxHealth;
    initted = true;
}

void Tank::render( int tankid ) const {
	if( !live )
		return;

    setColor(player->color);

	drawLinePath( getTankVertices( x, y, d ), 0.2, true );
};

void Tank::startNewMoveCycle() {
    CHECK(initted);
    timeLeft = 1;
};

void Tank::setKeys( const Keystates &keystates ) {
    keys = keystates;
};

void Tank::move() {
    move( timeLeft );
}

void Tank::move( Coord time ) {
	if( !live )
		return;
    
    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, x, y, d, time );
    
    x = newpos.first.x;
    y = newpos.first.y;
    d = newpos.second;
    
    timeLeft -= time;
	
};

void Tank::addCollision( Collider *collider ) const {
    
	if( !live )
		return;

	vector<Coord2> tankpts = getTankVertices( x, y, d );
    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, x, y, d, timeLeft );
	vector<Coord2> newtankpts = getTankVertices( newpos.first.x, newpos.first.y, newpos.second );
	for( int i = 0; i < newtankpts.size(); i++ )
		newtankpts[ i ] -= tankpts[ i ];
	for( int i = 0; i < 3; i++ )
		collider->token(Coord4(tankpts[i], tankpts[(i + 1) % 3]), Coord4(newtankpts[i], newtankpts[(i + 1) % 3]));
};

const float tank_width = 5;
const float tank_length = tank_width*1.3;

const Coord tank_coords[3][2] =  {
	{Coord(-tank_width / 2), Coord(-tank_length / 3)},
	{Coord(tank_width / 2), Coord(-tank_length / 3)},
	{Coord(0), Coord(tank_length * 2 / 3)}
};

vector<Coord2> Tank::getTankVertices( Coord tx, Coord ty, float td ) const {
	Coord xtx = cfcos( td );
	Coord xty = cfsin( td );
	Coord ytx = cfsin( td );
	Coord yty = -cfcos( td );
	vector<Coord2> rv;
	for( int i = 0; i < 3; i++ )
		rv.push_back(Coord2(tx + tank_coords[ i ][ 0 ] * xtx + tank_coords[ i ][ 1 ] * xty, ty + tank_coords[ i ][ 1 ] * yty + tank_coords[ i ][ 0 ] * ytx));
	return rv;
};

Coord2 Tank::getFiringPoint() const {
	Coord xtx = cfcos( d );
	Coord xty = cfsin( d );
	Coord ytx = cfsin( d );
	Coord yty = -cfcos( d );
	return Coord2( x + tank_coords[ 2 ][ 0 ] * xtx + tank_coords[ 2 ][ 1 ] * xty, y + tank_coords[ 2 ][ 1 ] * yty + tank_coords[ 2 ][ 0 ] * ytx );
};

pair<Coord2, float> Tank::getDeltaAfterMovement( const Keystates &keys, Coord x, Coord y, float d, Coord t ) const {
    
    float dv;
    float dd;
    if(keys.axmode == KSAX_UDLR) {
        dd = deadzone(keys.ax[0], keys.ax[1], 0.2, 0);
        dv = deadzone(keys.ax[1], keys.ax[0], 0.2, 0);
    } else if(keys.axmode == KSAX_ABSOLUTE) {
        float xpd = deadzone(keys.ax[0], keys.ax[1], 0, 0.2);
        float ypd = deadzone(keys.ax[1], keys.ax[0], 0, 0.2);
        if(xpd == 0 && ypd == 0) {
            dv = dd = 0;
        } else {
            float desdir = atan2(xpd, ypd);
            desdir -= d;
            desdir += 2 * PI;
            if(desdir > PI)
                desdir -= 2 * PI;
            dd = desdir / player->turnSpeed;
            if(dd < -1)
                dd = -1;
            if(dd > 1)
                dd = 1;
            dv = min(sqrt(xpd * xpd + ypd * ypd), 1.f);
            if(abs(desdir) > PI / 3 && abs(desdir) < PI / 3 * 2)
                dv = 0; // if we're near right angles, stop
            else if(abs(desdir) > PI / 2)
                dv = -dv;   // if we're merely backwards, go backwards
        }
    //} else if(keys.axmode == KSAX_TANK) {
        
    } else {
        CHECK(0);
    }
    
    CHECK(dv >= -1 && dv <= 1);
    CHECK(dd >= -1 && dd <= 1);
    
    Coord cdv(dv);

	x += player->maxSpeed * cdv * cfsin( d ) * t;
	y += -player->maxSpeed * cdv * cfcos( d ) * t;

	d += player->turnSpeed * dd * t.toFloat();
	d += 2*PI;
	d = fmod( d, 2*(float)PI );
    
    return make_pair( Coord2( x, y ), d );
    
}

bool Tank::takeDamage( float damage ) {
	health -= damage;
	if( health <= 0 && live ) {
		live = false;
		spawnShards = true;
        return true;
	}
    return false;
};

void Tank::genEffects( vector< GfxEffects > *gfxe ) {
	if( spawnShards ) {
		vector<Coord2> tv = getTankVertices( x, y, d );
		for( int i = 0; i < tv.size(); i++ ) {
			GfxEffects ngfe;
			ngfe.pos = Float4(tv[i].x.toFloat(), tv[i].y.toFloat(), tv[(i + 1) % tv.size()].x.toFloat(), tv[(i + 1) % tv.size()].y.toFloat());
			float cx = ( ngfe.pos.sx + ngfe.pos.ex ) / 2;
			float cy = ( ngfe.pos.sy + ngfe.pos.ey ) / 2;
			ngfe.vel.sx = ngfe.vel.ex = cx - x.toFloat();
			ngfe.vel.sy = ngfe.vel.ey = cy - y.toFloat();
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
	health = -47283;
    player = NULL;
    initted = false;
    weaponCooldown = 0;
}

void Projectile::startNewMoveCycle() {
    timeLeft = 1;
}
void Projectile::move() {
    move( timeLeft );
}
void Projectile::move( Coord time ) {
    x += v * cfsin( d ) * time;
    y += -v * cfcos( d ) * time;
    timeLeft -= time;
}

void Projectile::render() const {
	setColor( 1.0, 1.0, 1.0 );
	drawLine(Coord4(x, y, x + v * cfsin( d ), y - v * cfcos( d )), 0.1 );
};
void Projectile::addCollision( Collider *collider ) const {
    CHECK( timeLeft == 1 );
	collider->token( Coord4( x, y, x + v * cfsin( d ), y - v * cfcos( d ) ), Coord4( v * cfsin( d ), -v * cfcos( d ), v * cfsin( d ), -v * cfcos( d ) ) );
};
void Projectile::impact( Tank *target ) {
	if(target->takeDamage( damage ))
        owner->player->kills++;
    owner->player->damageDone += 1;
};

void Projectile::genEffects( vector< GfxEffects > *gfxe ) const {
    GfxEffects ngfe;
    ngfe.pos.sx = x.toFloat() + v.toFloat() * fsin( d );
    ngfe.pos.sy = y.toFloat() - v.toFloat() * fcos( d );
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
    {
        Float4 bounds = gamemap.getBounds().toFloat();
        expandBoundBox(&bounds, 1.1);
        float y = (bounds.ey - bounds.sy) / 0.9;
        float sy = bounds.ey - y;
        float sx =( bounds.sx + bounds.ex ) / 2 - ( y * 1.25 / 2 );
        setZoom(sx, sy, bounds.ey);
    }
	for( int i = 0; i < players.size(); i++ )
		players[ i ].render( i );
	for( int i = 0; i < projectiles.size(); i++ )
		for( int j = 0; j < projectiles[ i ].size(); j++ )
			projectiles[ i ][ j ].render();
	for( int i = 0; i < gfxeffects.size(); i++ )
		gfxeffects[ i ].render();
	gamemap.render();
	collider.render();
    {
        setZoom( 0, 0, 100 );
        setColor(1.0, 1.0, 1.0);
        drawLine(Float4(0, 10, 125, 10), 0.1);
        for(int i = 0; i < players.size(); i++) {
            setColor(1.0, 1.0, 1.0);
            float loffset = 125.0 / players.size() * i;
            float roffset = 125.0 / players.size() * ( i + 1 );
            if(i)
                drawLine(Float4(loffset, 0, loffset, 10), 0.1);
            if(players[i].live) {
                setColor(players[i].player->color);
                float barl = loffset + 1;
                float bare = (roffset - 1) - (loffset + 1);
                bare /= players[i].player->maxHealth;
                bare *= players[i].health;
                drawShadedBox(Float4(barl, 2, barl + bare, 6), 0.1, 2);
                drawText(StringPrintf("%d", players[i].player->shotsLeft), 2, barl, 7);
            }
        }
        if(frameNm < 180) {
            setColor(1.0, 1.0, 1.0);
            int fleft = 180 - frameNm;
            int s;
            if(frameNm % 60 < 5) {
                s = 15;
            } else if(frameNm % 30 < 5) {
                s = 12;
            } else {
                s = 8;
            }
            drawJustifiedText(StringPrintf("Ready %d.%02d", fleft / 60, fleft % 60), s, 125.0 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
        } else if(frameNm < 240) {
            float dens = (240.0 - frameNm) / 60;
            setColor(dens, dens, dens);
            drawJustifiedText("GO", 40, 125.0 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
        }
    }
};

void collideHandler( Collider *collider, vector< Tank > *tanks, const vector< Keystates > &keys, const vector< int > &tankx ) {
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf("Entering collide handler\n");
    
    vector< Keystates > stopped = keys;
    for( int i = 0; i < stopped.size(); i++ ) {
        stopped[ i ].u.down = false;
        stopped[ i ].d.down = false;
        stopped[ i ].l.down = false;
        stopped[ i ].r.down = false;
        stopped[ i ].ax[0] = 0;
        stopped[ i ].ax[1] = 0;
    }
    
    Tank temptank;
    
    Coord cCollideTimeStamp = collider->getCurrentTimestamp() + COLLISIONROLLBACK;
    
    Coord cTimeStamp = collider->getCurrentTimestamp();
    Coord rollbackStep = COLLISIONROLLBACK;
    
    {
        collider->setCurrentTimestamp( cCollideTimeStamp );
        CHECK( collider->testCollideAll() );
        collider->setCurrentTimestamp( cTimeStamp );
    }
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf( "Modified timestamp is %f, vs %f (%f diff)\n", cCollideTimeStamp.toFloat(), cTimeStamp.toFloat(), cCollideTimeStamp.toFloat() - cTimeStamp.toFloat() );
    
    while(1) {
        
        // If this triggers, we've gotten ourselves in a weird but potentially possible situation
        // The solution is to figure out *all* the tanks involved in the deadlock, and keep them all from moving
        // And possibly flash a "treads locked" message so they can figure out how to get out :P
        CHECK( cTimeStamp > 0 );
        
        cTimeStamp = max( cTimeStamp - rollbackStep, 0 );
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
        if(!ffwd && FLAGS_verboseCollisions)
            dprintf( "Multiple rollback! %f\n", rollbackStep.toFloat() );
        // roll back again
    }
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf( "Unfreeze timestamp is %f\n", cTimeStamp.toFloat() );
    
    /* now we have our real timestamp, so let's see what can be released */
    /* all the tanks are still in the "fixed" location", so let's roll forward to the collide point and see what it looks like then */
    
    collider->setCurrentTimestamp( cCollideTimeStamp );
    
    vector< char > mustBeFrozen( tankx.size() );
    
    for( int i = 0; i < tankx.size(); i++ ) {
        if(!ffwd && FLAGS_verboseCollisions)
            dprintf( "Retesting %d\n", i );
        // rewind, place tank in again
        collider->setCurrentTimestamp( 0 );
        temptank = (*tanks)[tankx[i]];
        collider->clearGroup(0, tankx[i]);
        collider->addThingsToGroup(0, tankx[i]);
        collider->startToken(0);
        temptank.addCollision( collider );
        collider->endAddThingsToGroup();
        collider->setCurrentTimestamp( cCollideTimeStamp );
        
        if(!ffwd && FLAGS_verboseCollisions)
            dprintf( "Clix\n" );
        // if it's colliding again, it clearly can't move forward
        if( collider->testCollideAgainst( tankx[ i ] ) )
            mustBeFrozen[ i ] = true;
        
        if(!ffwd && FLAGS_verboseCollisions)
            dprintf( "Reset\n" );
        
        temptank = (*tanks)[tankx[i]];
        temptank.move( cTimeStamp );
        temptank.setKeys( stopped[ i ] );
        collider->clearGroup(0, tankx[i]);
        collider->addThingsToGroup(0, tankx[i]);
        collider->startToken(0);
        temptank.addCollision( collider );
        collider->endAddThingsToGroup();
        if(!ffwd && FLAGS_verboseCollisions)
            dprintf( "Done\n" );
    }
    
    if(!ffwd && FLAGS_verboseCollisions)
        for( int i = 0; i < tankx.size(); i++ )
            dprintf( "Freeze tokens: %d is %d\n", tankx[ i ], (int)mustBeFrozen[ i ] );
    
    if( count( mustBeFrozen.begin(), mustBeFrozen.end(), false ) == mustBeFrozen.size() ) {
        if(!ffwd && FLAGS_verboseCollisions)
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
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf( "Sim continuing\n" );

    for(int i = 0; i < tankx.size(); i++)
        CHECK(!collider->testCollideAgainst( tankx[ i ] ));
    CHECK( !collider->testCollideAll(true) );
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf("Exiting collide handler\n");
    
}

bool Game::runTick( const vector< Keystates > &rkeys ) {
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf("Ticking\n");
    
	frameNm++;
    
    vector< Keystates > keys = rkeys;
    if(frameNm < 180) {
        for(int i = 0; i < keys.size(); i++) {
            keys[i].u = keys[i].d = keys[i].r = keys[i].l = keys[i].f = Button();
            keys[i].ax[0] = keys[i].ax[1] = 0;
        }
    }

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
    
    CHECK( !collider.testCollideAll(true) );
    
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
            if(!ffwd && FLAGS_verboseCollisions)
                dprintf( "Wall-tank collision\n" );
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
            if(!ffwd && FLAGS_verboseCollisions)
                dprintf( "Tank-tank collision\n" );
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
		players[ i ].move();
        players[ i ].weaponCooldown--;
    }

	for( int i = 0; i < players.size(); i++ ) {
		if( players[ i ].live && keys[ i ].f.down && players[ i ].weaponCooldown <= 0 ) {
            firepowerSpent +=players[ i ].player->weapon->costpershot;
			Projectile proj;
			proj.x = players[ i ].getFiringPoint().x;
			proj.y = players[ i ].getFiringPoint().y;
			proj.d = players[ i ].d;
            proj.v = players[ i ].player->weapon->projectile->velocity;
            proj.damage = players[ i ].player->weapon->projectile->damage;
            proj.owner = &players[ i ];
			projectiles[ i ].push_back( proj );
            players[ i ].weaponCooldown = players[ i ].player->weapon->firerate;
            if(players[i].player->shotsLeft != -1)
                players[i].player->shotsLeft--;
            if(players[i].player->shotsLeft == 0) {
                players[i].player->weapon = defaultWeapon();
                players[i].player->shotsLeft = -1;
            }
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

	for( int i = 0; i < players.size(); i++ ) {
        players[ i ].weaponCooldown--;
		players[ i ].genEffects( &gfxeffects );
    }
    
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
    
    if(framesSinceOneLeft / FPS >= 3) {
        for(int i = 0; i < players.size(); i++) {
            if(players[i].live)
                players[i].player->wins++;
        }
        return true;
    } else {
        return false;
    }

};

Game::Game() {
}

Game::Game(vector<Player> *in_playerdata, const Level &lev) {
    CHECK(in_playerdata);
	frameNm = 0;
    players.clear();
	players.resize( in_playerdata->size() );
    for(int i = 0; i < players.size(); i++) {
        players[i].init(&(*in_playerdata)[i]);
    }
    {
        // place players
        CHECK(lev.playerStarts.count(players.size()));
        vector<pair<Coord2, float> > pstart = lev.playerStarts.find(players.size())->second;
        for(int i = 0; i < pstart.size(); i++)
            dprintf("possible: %f, %f, %f\n", pstart[i].first.x.toFloat(), pstart[i].first.y.toFloat(), pstart[i].second);
        for(int i = 0; i < players.size(); i++) {
            int loc = int(frand() * pstart.size());
            CHECK(loc >= 0 && loc < pstart.size());
            dprintf("loc %d, %f %f %f\n", loc, pstart[loc].first.x.toFloat(), pstart[loc].first.y.toFloat(), pstart[loc].second);
            players[i].x = Coord(pstart[loc].first.x);
            players[i].y = Coord(pstart[loc].first.y);
            players[i].d = pstart[loc].second;
            pstart.erase(pstart.begin() + loc);
        }
    }

	projectiles.resize( in_playerdata->size() );
    framesSinceOneLeft = 0;
    firepowerSpent = 0;
    
    gamemap = Gamemap(lev);
};



