
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
    timeDone = 0;
};

void Tank::setKeys( const Keystates &keystates ) {
    keys = keystates;
};

void Tank::move() {
    move( 1 - timeDone );
}

void Tank::move( Coord time ) {
	if( !live )
		return;
    
    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, x, y, d, time );
    
    x = newpos.first.x;
    y = newpos.first.y;
    d = newpos.second;
    
    timeDone += time;
    
    CHECK(timeDone >= 0 && timeDone <= 1);
	
};

void Tank::moveTo(Coord time) {
    CHECK(time >= timeDone);
    move(time - timeDone);
};

void Tank::addCollision( Collider *collider ) const {
    
	if( !live )
		return;

	vector<Coord2> tankpts = getTankVertices( x, y, d );
    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, x, y, d, 1 - timeDone );
    CHECK(newpos.first.x == x && newpos.first.y == y && newpos.second == d || timeDone == 0);
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

bool Game::runTick( const vector< Keystates > &rkeys ) {
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf("Ticking\n");
    
	frameNm++;
    
    vector< Keystates > keys = rkeys;
    if(frameNm < 180) {
        for(int i = 0; i < keys.size(); i++) {
            keys[i].nullMove();
            keys[i].f = Button();
        }
    }
    
    // first we deal with moving the players around
    // we shuffle the player order randomly, then move players in that order
    // if the player can't move where they want to, they simply don't move
    // I am assuming there will be no tanks that can move further than their entire length per frame.
    
    // Second, we feed the player start/end positions into the old collision system, along with all the projectiles
    // And then we do our collision system solely on projectile/* intersections.
    
    // I think this works.
    
    vector<int> playerorder;
    {
        vector<int> playersleft;
        for(int i = 0; i < players.size(); i++)
            playersleft.push_back(i);
        while(playersleft.size()) {
            int pt = int(frand() * playersleft.size());
            CHECK(pt >= 0 && pt < playersleft.size());
            playerorder.push_back(playersleft[pt]);
            playersleft.erase(playersleft.begin() + pt);
        }
    }
    
    collider.reset(players.size());
    
    collider.addThingsToGroup(-1, 0);
    collider.startToken(0);
    gamemap.addCollide( &collider );
	collider.endAddThingsToGroup();
    
    collider.setCurrentTimestamp(0);
    
    for(int i = 0; i < playerorder.size(); i++) {
        collider.addThingsToGroup(0, i);
        collider.startToken(0);
        players[i].startNewMoveCycle();
        players[i].setKeys(Keystates());
        players[i].addCollision(&collider);
        collider.endAddThingsToGroup();
    }
    
    CHECK(!collider.testCollideAll(true));
    collider.setCurrentTimestamp(1);
    CHECK(!collider.testCollideAll(true));
    collider.setCurrentTimestamp(0);
    
    vector<bool> stopped(players.size());
    vector<bool> notstopped(players.size());
    
    //dprintf("Starting cycle\n");
    
    CHECK(playerorder.size() == players.size());
    for(int i = 0; i < playerorder.size(); i++) {
        CHECK(count(playerorder.begin(), playerorder.end(), playerorder[i]) == 1);
        CHECK(playerorder[i] >= 0 && playerorder[i] < players.size());
        
        //dprintf("PO %d, %d\n", i, playerorder[i]);
        
        collider.clearGroup(0, playerorder[i]);
        collider.addThingsToGroup(0, playerorder[i]);
        collider.startToken(0);
        Tank temptank = players[playerorder[i]];
        temptank.setKeys(keys[playerorder[i]]);
        temptank.addCollision(&collider);
        collider.endAddThingsToGroup();
        
        CHECK(!collider.testCollideAll(true));
    
        collider.setCurrentTimestamp(1);

        if(collider.testCollideAgainst(playerorder[i])) {
            //dprintf("Single collide hit!\n");
            collider.setCurrentTimestamp(0);
            keys[playerorder[i]].nullMove();
            collider.clearGroup(0, playerorder[i]);
            collider.addThingsToGroup(0, playerorder[i]);
            collider.startToken(0);
            players[playerorder[i]].setKeys(keys[playerorder[i]]);
            players[playerorder[i]].addCollision(&collider);
            collider.endAddThingsToGroup();
            stopped[playerorder[i]] = true;
        } else {
            //dprintf("Single collide didn't hit!\n");
            //CHECK(!collider.testCollideAll(true));
            collider.setCurrentTimestamp(0);
            players[playerorder[i]].setKeys(keys[playerorder[i]]);
            notstopped[playerorder[i]] = true;
        }

        //CHECK(!collider.testCollideAll(true));
        //collider.setCurrentTimestamp(1);
        //CHECK(!collider.testCollideAll(true));
        //collider.setCurrentTimestamp(0);

    }
    
    /*
    {
        string ope;
        for(int i = 0; i < stopped.size(); i++) {
            CHECK(stopped[i] + notstopped[i] == 1);
            ope += stopped[i] + '0';
        }
        dprintf("%s\n", ope.c_str());
    }
    */

	for( int j = 0; j < projectiles.size(); j++ ) {
		collider.addThingsToGroup(1, j);
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            collider.startToken(k);
            projectiles[ j ][ k ].startNewMoveCycle();
			projectiles[ j ][ k ].addCollision( &collider );
		}
		collider.endAddThingsToGroup();
	}
    
    CHECK(!collider.testCollideAll(true));
    
	while( collider.doProcess() ) {
		//dprintf( "Collision!\n" );
		//dprintf( "Timestamp %f\n", collider.getCurrentTimestamp().toFloat() );
		//dprintf( "%d,%d,%d vs %d,%d,%d\n", collider.getLhs().first.first, collider.getLhs().first.second, collider.getLhs().second, collider.getRhs().first.first, collider.getRhs().first.second, collider.getRhs().second );
        pair< pair< int, int >, int > lhs = collider.getLhs();
        pair< pair< int, int >, int > rhs = collider.getRhs();
        if( lhs > rhs ) swap( lhs, rhs );
        if( lhs.first.first == -1 && rhs.first.first == -1 ) {
            // wall-wall collision, wtf?
            CHECK(0);
        } else if( lhs.first.first == -1 && rhs.first.first == 0 ) {
            // wall-tank collision, should never happen
            CHECK(0);
        } else if( lhs.first.first == -1 && rhs.first.first == 1 ) {
            // wall-projectile collision - kill projectile
            collider.removeThingsFromGroup( rhs.first.first, rhs.first.second );
            collider.startToken(rhs.second);
            projectiles[ rhs.first.second ][ rhs.second ].addCollision( &collider );
            collider.endRemoveThingsFromGroup();
            projectiles[ rhs.first.second ][ rhs.second ].live = false;
            projectiles[ rhs.first.second ][ rhs.second ].genEffects( &gfxeffects );
        } else if( lhs.first.first == 0 && rhs.first.first == 0 ) {
            // tank-tank collision, should never happen
            CHECK(0);
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
            // nothing meaningful, should totally never happen, what the hell is going on here, who are you, and why are you in my apartment
            CHECK(0);
        }
	}
    
    CHECK(collider.getCurrentTimestamp() == 1);
    
    CHECK(!collider.testCollideAll(true));

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
        Coord prem = players[i].timeDone;
		players[ i ].move();
        if(players[i].live && players[i].timeDone != 1) {
            dprintf("%f, wtf?\n", players[i].timeDone.toFloat());
            dprintf("%x also\n", (unsigned int)players[i].timeDone.raw());
            dprintf("%f, wtf?\n", prem.toFloat());
            dprintf("%x also\n", (unsigned int)prem.raw());
            CHECK(0);
        }
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



