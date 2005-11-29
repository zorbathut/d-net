
#include "game.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
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
DEFINE_int(startingcash, 1000, "Cash to start with");

void dealDamage(float dmg, Tank *target, Player *owner, float damagecredit, bool killcredit) {
    if(target->player == owner)
        return; // friendly fire exception
    if(target->takeDamage(dmg) && killcredit)
        owner->kills++;
    owner->damageDone += dmg * damagecredit;
};

void detonateWarhead(const IDBWarhead *warhead, Coord2 pos, Tank *impact, Player *owner, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm, float damagecredit, bool killcredit) {
    
    if(impact)
        dealDamage(warhead->impactdamage, impact, owner, damagecredit, killcredit);
    
    for(int i = 0; i < adjacency.size(); i++) {
        if(adjacency[i].first < warhead->radiusfalloff)
            dealDamage(warhead->radiusdamage / warhead->radiusfalloff * ( warhead->radiusfalloff - adjacency[i].first), adjacency[i].second, owner, damagecredit, killcredit);
    }
    
    GfxEffects ngfe;
    ngfe.point_pos = pos.toFloat();
    ngfe.life = 6;
    ngfe.type = GfxEffects::EFFECT_POINT;
    ngfe.color = Color(1.0, 1.0, 1.0);
    for( int i = 0; i < 6; i++ ) {
        float dir = frand() * 2 * PI;
        ngfe.point_vel = makeAngle(dir) / 3;
        ngfe.point_vel *= 1.0 - frand() * frand();
        gfxe->push_back( ngfe );
    }
    
    if(warhead->radiusfalloff > 0) {
        GfxEffects dbgf;
        dbgf.type = GfxEffects::EFFECT_CIRCLE;
        dbgf.circle_center = pos.toFloat();
        dbgf.circle_radius = warhead->radiusfalloff;
        dbgf.life = 5;
        gfxe->push_back(dbgf);
    }
    
    if(warhead->wallremovalradius > 0 && frand() < warhead->wallremovalchance) {
        gm->removeWalls(pos, warhead->wallremovalradius);
    }

};

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
    CHECK(upg);
    return count(upgrades.begin(), upgrades.end(), upg);
}

int Player::resellAmmoValue() const {
    return int(shotsLeft * weapon->costpershot * 0.8);
}

Player::Player() {
    color = Color(0.5, 0.5, 0.5);
    cash = FLAGS_startingcash;
    reCalculate();
    weapon = defaultWeapon();
    glory = defaultGlory();
    bombardment = defaultBombardment();
    shotsLeft = -1;
}

void GfxEffects::move() {
    CHECK(life != -1);
	age++;
}
void GfxEffects::render() const {
    CHECK(life != -1);
    float apercent = 1.0f - (float)age / life;
	setColor(color * apercent);
    if(type == EFFECT_LINE) {
        drawLine(line_pos + line_vel * age, 0.1f);
    } else if(type == EFFECT_POINT) {
        drawPoint(point_pos.x + point_vel.x * age, point_pos.y + point_vel.y * age, 0.1f);
    } else if(type == EFFECT_CIRCLE) {
        drawCircle(circle_center, circle_radius, 0.1f);
    } else if(type == EFFECT_TEXT) {
        drawText(text_data, text_size, text_pos + text_vel * age);
    } else if(type == EFFECT_PATH) {
        drawTransformedLinePath(path_path, path_ang_start + path_ang_vel * age + path_ang_acc * age * age / 2, path_pos_start + path_pos_vel * age + path_pos_acc * age * age / 2, 0.1f);
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
    color = Color(1.0, 1.0, 1.0);
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

	drawLineLoop(getTankVertices(pos, d), 0.2);
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
    
    float dl;
    float dr;
     if(keys.axmode == KSAX_TANK) {
        dl = deadzone(keys.ax[0], keys.ax[1], 0.2, 0);
        dr = deadzone(keys.ax[1], keys.ax[0], 0.2, 0);
     } else if(keys.axmode == KSAX_ABSOLUTE || keys.axmode == KSAX_UDLR) {
        float dd;
        float dv;
        if(keys.axmode == KSAX_ABSOLUTE) {
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
        } else {
            dd = deadzone(keys.ax[0], keys.ax[1], 0.2, 0);
            dv = deadzone(keys.ax[1], keys.ax[0], 0.2, 0);
        }
        // What aspects do we want here?
        // If dv is zero, we turn at full speed.
        // If dd is zero, we move at full speed.
        // For neutral:
        //  1  1
        //  0  0
        // -1 -1
        // For left turn:
        //  0  1
        // -1  1
        // -1  0
        // For right turn:
        //  1  0
        //  1 -1
        //  0 -1
        float d00, d01, d02, d10, d11, d12;
        if(dd <= 0) {
            d00 = 1 + dd;
            d10 = 1;
            d01 = dd;
            d11 = -dd;
            d02 = -1;
            d12 = -1 - dd;
        } else {
            d00 = 1;
            d10 = 1 - dd;
            d01 = dd;
            d11 = -dd;
            d02 = -1 + dd;
            d12 = -1;
        }
        if(dv <= 0) {
            float intens = abs(dv);
            dl = (d02 * intens + d01 * (1 - intens));
            dr = (d12 * intens + d11 * (1 - intens));
        } else {
            float intens = abs(dv);
            dl = (d00 * intens + d01 * (1 - intens));
            dr = (d10 * intens + d11 * (1 - intens));
        }
    }

    float dv = (dr + dl) / 2;
    float dd = (dl - dr) / 2;
    
    // More random thoughts:
    // 1 and 0 should stay 1 and 0
    // 0.5 and 0 should stay 0.5 and 0
    // 0.5 and 0.5 should scale up (to 0.707 both?)
    
    Float2 ult = makeAngle(getAngle(Float2(dv, dd)));
    float dif = 1 / (abs(ult.x) + abs(ult.y));
    dv /= dif;
    dd /= dif;
    
    if(dv > 1) dv = 1;
    if(dd > 1) dd = 1;
    if(dv < -1) dv = -1;
    if(dd < -1) dd = -1;

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

void Tank::genEffects(vector<GfxEffects> *gfxe, vector<Projectile> *projectiles) {
	if( spawnShards ) {
		vector<Coord2> tv = getTankVertices( pos, d );
        Coord2 centr = getCentroid(tv);
        Coord tva = getArea(tv);
        
        for(int i = 0; i < tv.size(); i++)
            tv[i] -= centr;
        
        const IDBGlory *glory = player->glory;
        
        vector<float> ang;
        {
            int ct = int(frand() * (glory->maxsplits - glory->minsplits + 1)) + glory->minsplits;
            CHECK(ct <= glory->maxsplits && ct >= glory->minsplits);
            for(int i = 0; i < ct; i++)
                ang.push_back(frand() * (glory->maxsplitsize - glory->minsplitsize) + glory->minsplitsize);
            for(int i = 1; i < ang.size(); i++)
                ang[i] += ang[i - 1];
            float angtot = accumulate(ang.begin(), ang.end(), 0.0f);
            float shift = frand() * PI * 2;
            for(int i = 0; i < ang.size(); i++) {
                ang[i] *= PI * 2 / angtot;
                ang[i] += shift;
            }
        }
        
        vector<vector<Coord2> > chunks;
        for(int i = 0; i < ang.size(); i++) {
            int j = (i + 1) % ang.size();
            float ned = ang[j];
            if(ned > ang[i])
                ned -= 2 * PI;
            vector<Coord2> intersecty;
            intersecty.push_back(Coord2(0, 0));
            float kang = ang[i];
            do {
                intersecty.push_back(makeAngle(Coord(kang)) * 100);
                kang -= 0.5;
                if(kang - 0.25 < ned)
                    kang = ned;
            } while(kang > ned);
            intersecty.push_back(makeAngle(Coord(ned)) * 100);
            reverse(intersecty.begin(), intersecty.end());
            vector<vector<Coord2> > thischunk = getDifference(tv, intersecty);
            CHECK(thischunk.size() == 1);
            chunks.push_back(thischunk[0]);
        }
        
        for(int i = 0; i < chunks.size(); i++) {
            Coord2 subcentroid = getCentroid(chunks[i]);
            vector<Float2> vf2;
            for(int j = 0; j < chunks[i].size(); j++)
                vf2.push_back((chunks[i][j] - subcentroid).toFloat());
            Coord2 vel = normalize(subcentroid) / 10 * tva / getArea(chunks[i]);
            GfxEffects ngfe;
            ngfe.type = GfxEffects::EFFECT_PATH;
            ngfe.path_path = vf2;
            ngfe.path_pos_start = (centr + subcentroid).toFloat();
            ngfe.path_pos_vel = vel.toFloat();
            ngfe.path_pos_acc = -ngfe.path_pos_vel / 30;
            ngfe.path_ang_start = 0;
            ngfe.path_ang_vel = powerRand(2) / 20;
            ngfe.path_ang_acc = -ngfe.path_ang_vel / 30;
            ngfe.life = 30;
            ngfe.color = player->color;
            gfxe->push_back(ngfe);
        }
        
        for(int i = 0; i < ang.size(); i++)
            for(int j = 0; j < glory->shotspersplit; j++)
                projectiles->push_back(Projectile(centr, ang[i] + powerRand(2) / 10, glory->projectile, this));
        
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

void Projectile::tick(vector<GfxEffects> *gfxe) {
    CHECK(live);
    CHECK(age != -1);
    pos += movement();
    lasttail = nexttail();
    age++;
    
    if(projtype->motion == PM_NORMAL) {
    } else if(projtype->motion == PM_MISSILE) {
        if(age > 10)
            missile_sidedist /= 1.2;
        GfxEffects ngfe;
        ngfe.point_pos = pos.toFloat() + lasttail.toFloat() - movement().toFloat(); // projectiles get a free tick ATM
        ngfe.life = 10;
        ngfe.type = GfxEffects::EFFECT_POINT;
        ngfe.color = projtype->color;
        for( int i = 0; i < 2; i++ ) {
            float dir = frand() * 2 * PI;
            ngfe.point_vel = makeAngle(dir) / 3;
            ngfe.point_vel *= 1.0 - frand() * frand();
            ngfe.point_vel += movement().toFloat();
            ngfe.point_vel += missile_accel().toFloat() * -3 * abs(powerRand(2));
            ngfe.color = Color(1.0, 0.9, 0.6);
            gfxe->push_back( ngfe );
        }
    } else if(projtype->motion == PM_AIRBRAKE) {
        airbrake_velocity *= 0.95;
        if(airbrake_liveness() <= 0)
            live = false;
    } else {
        CHECK(0);
    }
}

void Projectile::render() const {
    CHECK(live);
    CHECK(age != -1);
    if(projtype->motion == PM_NORMAL) {
        setColor(projtype->color);
    } else if(projtype->motion == PM_MISSILE) {
        setColor(projtype->color);
    } else if(projtype->motion == PM_AIRBRAKE) {
        setColor(projtype->color * airbrake_liveness());
    } else {
        CHECK(0);
    }
    drawLine(Coord4(pos, pos + lasttail), 0.1);
};
void Projectile::addCollision( Collider *collider ) const {
    CHECK(live);
    collider->token( Coord4( pos, pos + lasttail ), Coord4( movement(), movement() + nexttail() ) );
};
void Projectile::impact(Coord2 pos, Tank *target, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm) {
    if(!live)
        return;
    
    detonateWarhead(projtype->warhead, pos, target, owner->player, adjacency, gfxe, gm, 1.0, true);

    live = false;
};

bool Projectile::isLive() const {
    return live;
}

Coord2 Projectile::movement() const {
    if(projtype->motion == PM_NORMAL) {
        return makeAngle(Coord(d)) * Coord(projtype->velocity);
    } else if(projtype->motion == PM_MISSILE) {
        return missile_accel() + missile_backdrop() + missile_sidedrop();
    } else if(projtype->motion == PM_AIRBRAKE) {
        return Coord2(makeAngle(d) * airbrake_velocity);
    } else {
        CHECK(0);
    }
    
}

Coord2 Projectile::nexttail() const {
    if(projtype->motion == PM_NORMAL) {
        return -movement();
    } else if(projtype->motion == PM_MISSILE) {
        return Coord2(makeAngle(d) * -2);
    } else if(projtype->motion == PM_AIRBRAKE) {
        return Coord2(-makeAngle(d) * (airbrake_velocity + 2));
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

float Projectile::airbrake_liveness() const {
    return 1.0 - (age / 60.0);
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
    } else if(projtype->motion == PM_AIRBRAKE) {
        airbrake_velocity = (powerRand(2) / 4 + 1) * projtype->velocity;
    } else {
        CHECK(0);
    }
}

// returns center and width/height
pair<Float2, Float2> getMapZoom(const Coord4 &mapbounds) {
    Float4 bounds = mapbounds.toFloat();
    pair<Float2, Float2> rv;
    rv.first.x = (bounds.sx + bounds.ex) / 2;
    rv.first.y = (bounds.sy + bounds.ey) / 2;
    rv.second.x = bounds.ex - bounds.sx;
    rv.second.y = bounds.ey - bounds.sy;
    rv.second.x *= 1.1;
    rv.second.y *= 1.1;
    return rv;
}

void goCloser(float *cur, const float *now, const float dist) {
    if(*cur < *now) {
        *cur = min(*cur + dist, *now);
    } else {
        *cur = max(*cur - dist, *now);
    }
}

void doInterp(float *curcenter, const float *nowcenter, float *curzoom, const float *nowzoom, float *curspeed) {
    if(*curcenter != *nowcenter || *curzoom != *nowzoom) {
        *curspeed = min(*curspeed + 0.0000002, 0.001);
        goCloser(curcenter, nowcenter, *nowzoom * *curspeed);
        goCloser(curzoom, nowzoom, *nowzoom * *curspeed);
    } else {
        *curspeed = max(*curspeed - 0.000002, 0.);
    }
}

bool Game::runTick( const vector< Keystates > &rkeys ) {
    
    if(!ffwd && FLAGS_verboseCollisions)
        dprintf("Ticking\n");
    
	frameNm++;
    
    tankHighlight.clear();
    tankHighlight.resize(players.size());
    
    vector< Keystates > keys = rkeys;
    if(frameNm < 180) {
        for(int i = 0; i < keys.size(); i++) {
            if(keys[i].f.down)
                tankHighlight[i] = true;
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
    
    const Coord4 gmb = gamemap.getBounds();
    collider.reset(players.size(), COM_PROJECTILE, gmb);
    
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
            projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
        } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PLAYER ) {
            // tank-tank collision, should never happen
            CHECK(0);
        } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PROJECTILE ) {
            // tank-projectile collision - kill projectile, do damage
            projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, &players[ lhs.bucket ], genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
        } else if( lhs.category == CGR_PROJECTILE && rhs.category == CGR_PROJECTILE ) {
            // projectile-projectile collision - kill both projectiles
            // also do radius damage, and do it fairly dammit
            bool lft = frand() < 0.5;
            
            if(lft)
                projectiles[ lhs.bucket ][ lhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
            
            projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
            
            if(!lft)
                projectiles[ lhs.bucket ][ lhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
            
        } else {
            // nothing meaningful, should totally never happen, what the hell is going on here, who are you, and why are you in my apartment
            CHECK(0);
        }
	}
    
    collider.finishProcess();

    {
        vector<vector<Projectile> > newProjectiles(projectiles.size());
        for(int j = 0; j < projectiles.size(); j++) {
            for(int k = 0; k < projectiles[ j ].size(); k++) {
                if(projectiles[j][k].isLive()) {
                    projectiles[j][k].tick(&gfxeffects);
                    if(!projectiles[j][k].isLive())   // in case it dies in its tick
                        continue;
                    newProjectiles[j].push_back(projectiles[j][k]);
                }
            }
        }
        projectiles.swap(newProjectiles);
    }
    
    for(int j = 0; j < bombards.size(); j++) {
        CHECK(bombards[j].state >= 0 && bombards[j].state < BombardmentState::BS_LAST);
        if(bombards[j].state == BombardmentState::BS_OFF) {
            if(!players[j].live) {
                // if the player is dead and the bombard isn't initialized
                bombards[j].loc = players[j].pos;
                bombards[j].state = BombardmentState::BS_SPAWNING;
                bombards[j].timer = 60 * 6;
            }
        } else if(bombards[j].state == BombardmentState::BS_SPAWNING) {
            bombards[j].timer--;
            if(bombards[j].timer <= 0)
                bombards[j].state = BombardmentState::BS_ACTIVE;
        } else if(bombards[j].state == BombardmentState::BS_ACTIVE) {
            bombards[j].loc.x += Coord(deadzone(keys[j].udlrax[0], keys[j].udlrax[1], 0, 0.2));
            bombards[j].loc.y += Coord(-deadzone(keys[j].udlrax[1], keys[j].udlrax[0], 0, 0.2));
            bombards[j].loc.x = max(bombards[j].loc.x, gmb.sx);
            bombards[j].loc.y = max(bombards[j].loc.y, gmb.sy);
            bombards[j].loc.x = min(bombards[j].loc.x, gmb.ex);
            bombards[j].loc.y = min(bombards[j].loc.y, gmb.ey);
            if(keys[j].f.down) {
                bombards[j].state = BombardmentState::BS_FIRING;
                bombards[j].timer = players[j].player->bombardment->lockdelay;
            }
        } else if(bombards[j].state == BombardmentState::BS_FIRING) {
            bombards[j].timer--;
            if(bombards[j].timer <= 0) {
                detonateWarhead(players[j].player->bombardment->warhead, bombards[j].loc, NULL, players[j].player, genTankDistance(bombards[j].loc), &gfxeffects, &gamemap, 1.0, false);
                bombards[j].state = BombardmentState::BS_COOLDOWN;
                bombards[j].timer = players[j].player->bombardment->unlockdelay;
            }
        } else if(bombards[j].state == BombardmentState::BS_COOLDOWN) {
            bombards[j].timer--;
            if(bombards[j].timer <= 0)
                bombards[j].state = BombardmentState::BS_ACTIVE;
        } else {
            CHECK(0);
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
            {
                string slv = StringPrintf("%d", players[i].player->shotsLeft);
                if(count(slv.begin(), slv.end(), '0') == slv.size() - 1) {
                    GfxEffects nge;
                    nge.type = GfxEffects::EFFECT_TEXT;
                    nge.life = 30;
                    nge.text_pos = players[i].pos.toFloat() + Float2(4, -4);
                    nge.text_vel = Float2(0, -0.1);
                    nge.text_size = 2.5;
                    nge.text_data = slv;
                    gfxeffects.push_back(nge);
                }
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
		players[ i ].genEffects(&gfxeffects, &projectiles[i]);
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

    {
        pair<Float2, Float2> z = getMapZoom(gamemap.getBounds());
        
        doInterp(&zoom_center.x, &z.first.x, &zoom_size.x, &z.second.x, &zoom_speed.x);
        doInterp(&zoom_center.y, &z.first.y, &zoom_size.y, &z.second.y, &zoom_speed.y);
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
        if(ais[i]) {
            if(players[i].live)
                ais[i]->updateGame(gamemap.getCollide(), players, i);
            else
                ais[i]->updateBombardment(players, bombards[i].loc);
        }
    }
}

void drawCirclePieces(const Coord2 &cloc, float solidity, float rad) {
    Float2 loc = cloc.toFloat();
    for(int i = 0; i < 3; i++) {
        float core = PI / 2 * 3 + (PI * 2 / 3 * i);
        float span = PI * 2 / 6 + PI * 2 / 6 * solidity;
        vector<Float2> path;
        for(int k = 0; k <= 10; k++)
            path.push_back(makeAngle(core - span / 2 + span / 10 * k) * rad + loc);
        drawLinePath(path, 0.1);
    }
}

void drawCrosses(const Coord2 &cloc, float rad) {
    Float2 loc = cloc.toFloat();
    for(int i = 0; i < 3; i++) {
        float core = PI / 2 * 3 + (PI * 2 / 3 * i);
        drawLine(makeAngle(core) * rad * 1.25 + loc, makeAngle(core) * rad * 0.75 + loc, 0.1);
    }
}

void Game::renderToScreen() const {
    {
        const float availScreen = 0.9;
        float pzoom = max(zoom_size.y / availScreen, zoom_size.x / 4 * 3);
        Float2 origin(zoom_center.x - pzoom * 4 / 3 / 2, zoom_center.y - pzoom * (1.0 - availScreen / 2));
        setZoom(origin.x, origin.y, origin.y + pzoom);
    }
	for( int i = 0; i < players.size(); i++ ) {
		players[ i ].render( i );
        if(tankHighlight[i]) {
            setColor(1.0, 1.0, 1.0);
            drawJustifiedText(StringPrintf("P%d", i), 2, players[i].pos.x.toFloat(), players[i].pos.y.toFloat(), TEXT_CENTER, TEXT_CENTER);
        }
    }
	for( int i = 0; i < projectiles.size(); i++ )
		for( int j = 0; j < projectiles[ i ].size(); j++ )
			projectiles[ i ][ j ].render();
    for( int i = 0; i < gfxeffects.size(); i++ )
		gfxeffects[ i ].render();
    for(int i = 0; i < bombards.size(); i++) {
        if(bombards[i].state == BombardmentState::BS_OFF) {
        } else if(bombards[i].state == BombardmentState::BS_SPAWNING) {
        } else if(bombards[i].state == BombardmentState::BS_ACTIVE) {
            setColor(players[i].player->color * 0.5);
            drawCirclePieces(bombards[i].loc, 0.3, 4);
            drawCrosses(bombards[i].loc, 4);
        } else if(bombards[i].state == BombardmentState::BS_FIRING) {
            setColor(players[i].player->color * 0.25);
            drawCirclePieces(bombards[i].loc, 0.3, 4);
            drawCrosses(bombards[i].loc, 4);
            setColor(Color(1.0, 1.0, 1.0));
            float ps = (float)bombards[i].timer / players[i].player->bombardment->lockdelay;
            drawCirclePieces(bombards[i].loc, 1 - ps, 4 * ps);
        } else if(bombards[i].state == BombardmentState::BS_COOLDOWN) {
            setColor(players[i].player->color * 0.25);
            drawCirclePieces(bombards[i].loc, 0.3, 4);
            drawCrosses(bombards[i].loc, 4);
            float ps = (float)bombards[i].timer / players[i].player->bombardment->unlockdelay;
            drawCirclePieces(bombards[i].loc, ps, 4 * (1 - ps));
        } else {
            CHECK(0);
        }
    }
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
                drawShadedRect(Float4(barl, 2, barl + bare, 6), 0.1, 2);
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
            drawJustifiedText(StringPrintf("Ready %d.%02d", fleft / 60, fleft % 60), s, 133.3 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
        } else if(frameNm < 240) {
            float dens = (240.0 - frameNm) / 60;
            setColor(dens, dens, dens);
            drawJustifiedText("GO", 40, 133.3 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
        }
    }
};

Game::Game() {
}

Game::Game(vector<Player> *in_playerdata, const Level &lev) {
    CHECK(in_playerdata);
	frameNm = 0;
    players.clear();
	players.resize(in_playerdata->size());
    bombards.resize(in_playerdata->size());
    for(int i = 0; i < players.size(); i++) {
        players[i].init(&(*in_playerdata)[i]);
    }
    {
        // place players
        CHECK(lev.playerStarts.count(players.size()));
        vector<pair<Coord2, float> > pstart = lev.playerStarts.find(players.size())->second;
        for(int i = 0; i < players.size(); i++) {
            int loc = int(frand() * pstart.size());
            CHECK(loc >= 0 && loc < pstart.size());
            players[i].pos = Coord2(pstart[loc].first);
            players[i].d = pstart[loc].second;
            pstart.erase(pstart.begin() + loc);
        }
    }

	projectiles.resize( in_playerdata->size() );
    framesSinceOneLeft = 0;
    firepowerSpent = 0;
    
    tankHighlight.resize(players.size());
    
    gamemap = Gamemap(lev);
    
    pair<Float2, Float2> z = getMapZoom(gamemap.getBounds());
    zoom_center = z.first;
    zoom_size = z.second;
    
    zoom_speed = Float2(0, 0);

};

vector<pair<float, Tank *> > Game::genTankDistance(const Coord2 &center) {
    vector<pair<float, Tank *> > rv;
    for(int i = 0; i < players.size(); i++) {
        if(players[i].live) {
            vector<Coord2> tv = players[i].getTankVertices(players[i].pos, players[i].d);
            if(inPath(center, tv)) {
                rv.push_back(make_pair(0, &players[i]));
                continue;
            }
            float closest = 1e10;
            for(int j = 0; j < tv.size(); j++) {
                float tdist = distanceFromLine(Coord4(tv[j], tv[(j + 1) % tv.size()]), center).toFloat();
                if(tdist < closest)
                    closest = tdist;
            }
            CHECK(closest < 1e10);
            CHECK(closest >= 0);
            rv.push_back(make_pair(closest, &players[i]));
        }
    }
    return rv;
}
