
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
#include "ai.h"

DEFINE_bool(verboseCollisions, false, "Verbose collisions");
DEFINE_bool(debugGraphics, false, "Enable various debug graphics");

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

bool Player::hasUpgrade(const IDBUpgrade *upg) const {
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
    CHECK(life != -1);
	age++;
}
void GfxEffects::render() const {
    CHECK(life != -1);
    float apercent = 1.0f - (float)age / life;
	setColor(apercent, apercent, apercent);
    if(type == EFFECT_LINE) {
        drawLine(line_pos + line_vel * age, 0.1f);
    } else if(type == EFFECT_POINT) {
        drawPoint(point_pos.x + point_vel.x * age, point_pos.y + point_vel.y * age, 0.1f);
    } else if(type == EFFECT_CIRCLE) {
        drawCircle(circle_center, circle_radius, 0.1f);
    } else {
        CHECK(0);
    }
}
bool GfxEffects::dead() const {
	return age >= life;
}


GfxEffects::GfxEffects() {
	age = 0;
    life = -1;
}

void Tank::init(Player *in_player) {
    CHECK(in_player);
    CHECK(!player);
    player = in_player;
    health = player->maxHealth;
    initted = true;
}

void Tank::tick(const Keystates &kst) {
    
    pair<Coord2, float> newpos = getDeltaAfterMovement( kst, pos, d );
    
    pos = newpos.first;
    d = newpos.second;
	
};

void Tank::render( int tankid ) const {
	if( !live )
		return;

    setColor(player->color);

	drawLinePath( getTankVertices( pos, d ), 0.2, true );
};

vector<Coord4> Tank::getCurrentCollide() const {
	if( !live )
		return vector<Coord4>();

	vector<Coord2> tankpts = getTankVertices( pos, d );
    vector<Coord4> rv;
    for(int i = 0; i < tankpts.size(); i++) {
        int j = (i + 1) % tankpts.size();
        rv.push_back(Coord4(tankpts[i], tankpts[j]));
    }
    return rv;
};

vector<Coord4> Tank::getNextCollide(const Keystates &keys) const {
	if( !live )
		return vector<Coord4>();

    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, pos, d );
	vector<Coord2> tankpts = getTankVertices( newpos.first, newpos.second );
    vector<Coord4> rv;
    for(int i = 0; i < tankpts.size(); i++) {
        int j = (i + 1) % tankpts.size();
        rv.push_back(Coord4(tankpts[i], tankpts[j]));
    }
    return rv;
};

void Tank::addCollision(Collider *collider, const Keystates &keys) const {
    
	if( !live )
		return;

	vector<Coord2> tankpts = getTankVertices( pos, d );
    pair<Coord2, float> newpos = getDeltaAfterMovement( keys, pos, d );
	vector<Coord2> newtankpts = getTankVertices( newpos.first, newpos.second );
	for( int i = 0; i < newtankpts.size(); i++ )
		newtankpts[ i ] -= tankpts[ i ];
	for( int i = 0; i < 3; i++ )
		collider->token(Coord4(tankpts[i], tankpts[(i + 1) % 3]), Coord4(newtankpts[i], newtankpts[(i + 1) % 3]));
};

const float tank_width = 5;
const float tank_length = tank_width*1.3;

const Coord tank_coords[3][2] =  {
	{Coord(-tank_length / 3), Coord(-tank_width / 2)},
	{Coord(-tank_length / 3), Coord(tank_width / 2)},
	{Coord(tank_length * 2 / 3), Coord(0)}
};

vector<Coord2> Tank::getTankVertices( Coord2 pos, float td ) const {
	Coord2 xt = makeAngle(Coord(td));
	Coord2 yt = makeAngle(Coord(td) - COORDPI / 2);
	vector<Coord2> rv;
	for( int i = 0; i < 3; i++ )
		rv.push_back(Coord2(pos.x + tank_coords[ i ][ 0 ] * xt.x + tank_coords[ i ][ 1 ] * xt.y, pos.y + tank_coords[ i ][ 1 ] * yt.y + tank_coords[ i ][ 0 ] * yt.x));
	return rv;
};

Coord2 Tank::getFiringPoint() const {
    Coord2 xt = makeAngle(Coord(d));
	Coord2 yt = makeAngle(Coord(d) - COORDPI / 2);
	return Coord2( pos.x + tank_coords[ 2 ][ 0 ] * xt.x + tank_coords[ 2 ][ 1 ] * xt.y, pos.y + tank_coords[ 2 ][ 1 ] * yt.y + tank_coords[ 2 ][ 0 ] * yt.x );
};

pair<Coord2, float> Tank::getDeltaAfterMovement( const Keystates &keys, Coord2 pos, float d ) const {
    
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
            float desdir = atan2(-ypd, xpd);
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
    } else if(keys.axmode == KSAX_TANK) {
        dv = 0;
        dd = 0;
    } else {
        CHECK(0);
    }
    
    CHECK(dv >= -1 && dv <= 1);
    CHECK(dd >= -1 && dd <= 1);
    
    Coord cdv(dv);

	pos += makeAngle(Coord(d)) * player->maxSpeed * cdv;

	d += player->turnSpeed * dd;
	d += 2*PI;
	d = fmod( d, 2*(float)PI );
    
    return make_pair( pos, d );
    
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
		vector<Coord2> tv = getTankVertices( pos, d );
		for( int i = 0; i < tv.size(); i++ ) {
			GfxEffects ngfe;
			ngfe.line_pos = Float4(tv[i].x.toFloat(), tv[i].y.toFloat(), tv[(i + 1) % tv.size()].x.toFloat(), tv[(i + 1) % tv.size()].y.toFloat());
			float cx = ( ngfe.line_pos.sx + ngfe.line_pos.ex ) / 2;
			float cy = ( ngfe.line_pos.sy + ngfe.line_pos.ey ) / 2;
			ngfe.line_vel.sx = ngfe.line_vel.ex = cx - pos.x.toFloat();
			ngfe.line_vel.sy = ngfe.line_vel.ey = cy - pos.y.toFloat();
			ngfe.line_vel /= 5;
			ngfe.life = 15;
            ngfe.type = GfxEffects::EFFECT_LINE;
			gfxe->push_back( ngfe );
		}
		spawnShards = false;
	}
}

Tank::Tank() {
	pos = Coord2(0, 0);
	d = 0;
	live = true;
	spawnShards = false;
	health = -47283;
    player = NULL;
    initted = false;
    weaponCooldown = 0;
}

void Projectile::tick() {
    CHECK(age != -1);
    pos += movement();
    lasttail = nexttail();
    age++;
    
    if(projtype->motion == PM_NORMAL) {
    } else if(projtype->motion == PM_MISSILE) {
        if(age > 10)
            missile_sidedist /= 1.2;
    } else {
        CHECK(0);
    }
}

void Projectile::render() const {
    CHECK(age != -1);
	setColor( 1.0, 1.0, 1.0 );
	drawLine(Coord4(pos, pos + lasttail), 0.1 );
};
void Projectile::addCollision( Collider *collider ) const {
    collider->token( Coord4( pos, pos + lasttail ), Coord4( movement(), movement() + nexttail() ) );
};
void Projectile::impact(Tank *target, const vector<pair<float, Tank *> > &adjacency) {
    if(target)
        dealDamage(projtype->warhead->impactdamage, target);
    
    for(int i = 0; i < adjacency.size(); i++) {
        if(adjacency[i].first < projtype->warhead->radiusfalloff)
            dealDamage(projtype->warhead->radiusdamage / projtype->warhead->radiusfalloff * ( projtype->warhead->radiusfalloff - adjacency[i].first), adjacency[i].second);
    }
};

void Projectile::dealDamage(float dmg, Tank *target) {
    if(target == owner)
        return; // friendly fire exception
    if(target->takeDamage(dmg))
        owner->player->kills++;
    owner->player->damageDone += dmg;
};

void Projectile::genEffects( vector< GfxEffects > *gfxe, Coord2 loc ) const {
    GfxEffects ngfe;
    ngfe.point_pos = loc.toFloat();
    ngfe.life = 10;
    ngfe.type = GfxEffects::EFFECT_POINT;
    for( int i = 0; i < 3; i++ ) {
        float dir = frand() * 2 * PI;
        ngfe.point_vel = makeAngle(dir) / 5;
        ngfe.point_vel *= 1.0 - frand() * frand();
        gfxe->push_back( ngfe );
    }
    
    if(projtype->warhead->radiusfalloff > 0) {
        GfxEffects dbgf;
        dbgf.type = GfxEffects::EFFECT_CIRCLE;
        dbgf.circle_center = loc.toFloat();
        dbgf.circle_radius = projtype->warhead->radiusfalloff;
        dbgf.life = 5;
        gfxe->push_back(dbgf);
    }
}

Coord2 Projectile::movement() const {
    if(projtype->motion == PM_NORMAL) {
        return makeAngle(Coord(d)) * Coord(projtype->velocity);
    } else if(projtype->motion == PM_MISSILE) {
        return missile_accel() + missile_backdrop() + missile_sidedrop();
    } else {
        CHECK(0);
    }
    
}

Coord2 Projectile::nexttail() const {
    if(projtype->motion == PM_NORMAL) {
        return -movement();
    } else if(projtype->motion == PM_MISSILE) {
        return -missile_accel();
    } else {
        CHECK(0);
    }
}

Coord2 Projectile::missile_accel() const {
    return makeAngle(Coord(d)) * Coord(projtype->velocity) * age / 60;
}
Coord2 Projectile::missile_backdrop() const {
    return makeAngle(Coord(d)) / 120;
}
Coord2 Projectile::missile_sidedrop() const {
    return makeAngle(Coord(d) - COORDPI / 2) * Coord(missile_sidedist);
}

Projectile::Projectile() {
    live = false;
    age = -1;
}
Projectile::Projectile(const Coord2 &in_pos, float in_d, const IDBProjectile *in_projtype, Tank *in_owner) {
    pos = in_pos;
    d = in_d;
    projtype = in_projtype;
    owner = in_owner;
    age = 0;
    live = true;
    lasttail = Coord2(0, 0);
    
    if(projtype->motion == PM_NORMAL) {
    } else if(projtype->motion == PM_MISSILE) {
        missile_sidedist = powerRand(2) * 0.25;
    } else {
        CHECK(0);
    }
}

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
    
    {
        
        collider.reset(players.size(), COM_PLAYER, gamemap.getBounds());
        
        collider.addThingsToGroup(CGR_WALL, 0);
        collider.startToken(0);
        gamemap.addCollide(&collider);
        collider.endAddThingsToGroup();
        
        for(int j = 0; j < players.size(); j++) {
            collider.addThingsToGroup(CGR_PLAYER, j);
            collider.startToken(0);
            players[j].addCollision(&collider, keys[j]);
            collider.endAddThingsToGroup();
        }
        
        collider.processSimple();
        
        CHECK(!collider.next());
        
        collider.finishProcess();
        
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
        
        CHECK(playerorder.size() == players.size());
        for(int i = 0; i < playerorder.size(); i++) {
            
            CHECK(count(playerorder.begin(), playerorder.end(), playerorder[i]) == 1);
            CHECK(playerorder[i] >= 0 && playerorder[i] < players.size());
            
            vector<Coord4> newpos = players[playerorder[i]].getNextCollide(keys[playerorder[i]]);
            
            if(collider.checkSimpleCollision(CGR_PLAYER, playerorder[i], newpos)) {
                keys[playerorder[i]].nullMove();
            } else {
                collider.clearGroup(CGR_PLAYER, playerorder[i]);
                collider.addThingsToGroup(CGR_PLAYER, playerorder[i]);
                collider.startToken(0);
                for(int j = 0; j < newpos.size(); j++)
                    collider.token(newpos[j], Coord4(0, 0, 0, 0));
                collider.endAddThingsToGroup();
            }

        }
        
    }
    
    collider.reset(players.size(), COM_PROJECTILE, gamemap.getBounds());
    
    // stuff!
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
    
    collider.addThingsToGroup(CGR_WALL, 0);
    collider.startToken(0);
    gamemap.addCollide(&collider);
    collider.endAddThingsToGroup();
    
    for(int j = 0; j < players.size(); j++) {
        collider.addThingsToGroup(CGR_PLAYER, j);
        collider.startToken(0);
        players[j].addCollision(&collider, keys[j]);
        collider.endAddThingsToGroup();
    }

	for( int j = 0; j < projectiles.size(); j++ ) {
		collider.addThingsToGroup(CGR_PROJECTILE, j);
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            collider.startToken(k);
			projectiles[ j ][ k ].addCollision( &collider );
		}
		collider.endAddThingsToGroup();
	}
    
    collider.processMotion();
    
	while( collider.next() ) {
		//dprintf( "Collision!\n" );
		//dprintf( "Timestamp %f\n", collider.getCurrentTimestamp().toFloat() );
		//dprintf( "%d,%d,%d vs %d,%d,%d\n", collider.getLhs().first.first, collider.getLhs().first.second, collider.getLhs().second, collider.getRhs().first.first, collider.getRhs().first.second, collider.getRhs().second );
        CollideId lhs = collider.getData().lhs;
        CollideId rhs = collider.getData().rhs;
        if( lhs > rhs ) swap( lhs, rhs );
        if( lhs.category == CGR_WALL && rhs.category == CGR_WALL ) {
            // wall-wall collision, wtf?
            CHECK(0);
        } else if( lhs.category == CGR_WALL && rhs.category == CGR_PLAYER ) {
            // wall-tank collision, should never happen
            CHECK(0);
        } else if( lhs.category == CGR_WALL && rhs.category == CGR_PROJECTILE ) {
            // wall-projectile collision - kill projectile
            projectiles[ rhs.bucket ][ rhs.item ].impact( NULL, genTankDistance(collider.getData().loc) );
            projectiles[ rhs.bucket ][ rhs.item ].live = false;
            projectiles[ rhs.bucket ][ rhs.item ].genEffects( &gfxeffects, collider.getData().loc );
        } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PLAYER ) {
            // tank-tank collision, should never happen
            CHECK(0);
        } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PROJECTILE ) {
            // tank-projectile collision - kill projectile, do damage
            projectiles[ rhs.bucket ][ rhs.item ].impact( &players[ lhs.bucket ], genTankDistance(collider.getData().loc) );
            projectiles[ rhs.bucket ][ rhs.item ].live = false;
            projectiles[ rhs.bucket ][ rhs.item ].genEffects( &gfxeffects, collider.getData().loc );
        } else if( lhs.category == CGR_PROJECTILE && rhs.category == CGR_PROJECTILE ) {
            // projectile-projectile collision - kill both projectiles
            // also do radius damage, and do it fairly dammit
            bool lft = frand() < 0.5;
            
            if(lft) {
                projectiles[ lhs.bucket ][ lhs.item ].impact( NULL, genTankDistance(collider.getData().loc) );
                projectiles[ lhs.bucket ][ lhs.item ].live = false;
                projectiles[ lhs.bucket ][ lhs.item ].genEffects( &gfxeffects, collider.getData().loc );
            }
            
            projectiles[ rhs.bucket ][ rhs.item ].impact( NULL, genTankDistance(collider.getData().loc) );
            projectiles[ rhs.bucket ][ rhs.item ].live = false;
            projectiles[ rhs.bucket ][ rhs.item ].genEffects( &gfxeffects, collider.getData().loc );
            
            if(!lft) {
                projectiles[ lhs.bucket ][ lhs.item ].impact( NULL, genTankDistance(collider.getData().loc) );
                projectiles[ lhs.bucket ][ lhs.item ].live = false;
                projectiles[ lhs.bucket ][ lhs.item ].genEffects( &gfxeffects, collider.getData().loc );
            }
        } else {
            // nothing meaningful, should totally never happen, what the hell is going on here, who are you, and why are you in my apartment
            CHECK(0);
        }
	}
    
    collider.finishProcess();

	for( int j = 0; j < projectiles.size(); j++ ) {
		for( int k = 0; k < projectiles[ j ].size(); k++ ) {
            if( !projectiles[ j ][ k ].live ) {
                projectiles[ j ].erase( projectiles[ j ].begin() + k );
                k--;
            } else {
            	projectiles[ j ][ k ].tick();
            }
        }
    }
    
	for( int i = 0; i < players.size(); i++ ) {
		players[ i ].tick(keys[i]);
        players[ i ].weaponCooldown--;
    }

	for( int i = 0; i < players.size(); i++ ) {
		if( players[ i ].live && keys[ i ].f.down && players[ i ].weaponCooldown <= 0 ) {
            firepowerSpent +=players[ i ].player->weapon->costpershot;
			projectiles[ i ].push_back(Projectile(players[ i ].getFiringPoint(), players[ i ].d + players[i].player->weapon->deploy->anglevariance * powerRand(2), players[ i ].player->weapon->projectile, &players[ i ]));
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

void Game::ai(const vector<Ai *> &ais) const {
    CHECK(ais.size() == players.size());
    for(int i = 0; i < ais.size(); i++) {
        if(ais[i])
            ais[i]->updateGame(gamemap.getCollide(), players, i);
    }
}

void Game::renderToScreen() const {
    {
        Float4 bounds = gamemap.getBounds().toFloat();
        expandBoundBox(&bounds, 1.1);
        float y = (bounds.ey - bounds.sy) / 0.9;
        float sy = bounds.ey - y;
        float sx = ( bounds.sx + bounds.ex ) / 2 - ( y * 4 / 3 / 2 );
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
        drawLine(Float4(0, 10, getZoomEx(), 10), 0.1);
        for(int i = 0; i < players.size(); i++) {
            setColor(1.0, 1.0, 1.0);
            float loffset = getZoomEx() / players.size() * i;
            float roffset = getZoomEx() / players.size() * ( i + 1 );
            if(i)
                drawLine(Float4(loffset, 0, loffset, 10), 0.1);
            if(players[i].live) {
                setColor(players[i].player->color);
                float barl = loffset + 1;
                float bare = (roffset - 1) - (loffset + 1);
                bare /= players[i].player->maxHealth;
                bare *= players[i].health;
                drawShadedBox(Float4(barl, 2, barl + bare, 6), 0.1, 2);
                string ammotext;
                if(players[i].player->shotsLeft == -1) {
                    ammotext = "inf";
                } else {
                    ammotext = StringPrintf("%d", players[i].player->shotsLeft);
                }
                drawText(ammotext, 2, barl, 7);
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
            players[i].pos = Coord2(pstart[loc].first);
            players[i].d = pstart[loc].second;
            pstart.erase(pstart.begin() + loc);
        }
    }

	projectiles.resize( in_playerdata->size() );
    framesSinceOneLeft = 0;
    firepowerSpent = 0;
    
    gamemap = Gamemap(lev);
};

vector<pair<float, Tank *> > Game::genTankDistance(const Coord2 &center) {
    vector<pair<float, Tank *> > rv;
    for(int i = 0; i < players.size(); i++) {
        if(players[i].live) {
            vector<Coord2> tv = players[i].getTankVertices(players[i].pos, players[i].d);
            float closest = 1e10;
            for(int j = 0; j < tv.size(); j++)
                if(len(center - tv[j]).toFloat() < closest)
                    closest = len(center - tv[j]).toFloat();
            CHECK(closest < 1e10);
            CHECK(closest >= 0);
            rv.push_back(make_pair(closest, &players[i]));
        }
    }
    return rv;
}
