#ifndef DNET_GAME
#define DNET_GAME

#include "gamemap.h"
#include "gfx.h"
#include "itemdb.h"
#include "coord.h"

#include <vector>
using namespace std;

class Collider;
class Ai;

#define RENDERTARGET_SPECTATOR -1

class Player {
public:

    float maxHealth;
    float turnSpeed;
    Coord maxSpeed;

    vector<const IDBUpgrade *> upgrades;
    const IDBWeapon *weapon;
    int shotsLeft;

    Color color;
    Dvec2 faction_symb;

    int cash;

    void reCalculate();
    bool hasUpgrade(const IDBUpgrade *upg) const;

    int resellAmmoValue() const;

    float damageDone;
    int kills;
    int wins;

    Player();

};

class GfxEffects {
public:

	void move();
	void render() const;
	bool dead() const;

	Float4 pos;
	Float4 vel;
	int life;

	int age;

    int type;
    enum { EFFECT_POINT, EFFECT_LINE };

	GfxEffects();

};

class Tank {
public:
    
    void init(Player *player);
    
	void tick(const Keystates &kst);
	void render( int tankid ) const;

	void addCollision( Collider *collider, const Keystates &kst ) const;

	Tank();

    vector<Coord4> getCurrentCollide() const;
    vector<Coord4> getNextCollide(const Keystates &keys) const;

	vector<Coord2> getTankVertices( Coord2 pos, float td ) const;
	Coord2 getFiringPoint() const;

    pair<Coord2, float> getDeltaAfterMovement( const Keystates &keys, Coord2 pos, float d ) const;

	bool takeDamage( float amount ); // returns true on kill
	void genEffects( vector< GfxEffects > *gfxe );
    
    bool initted;
	
	Coord2 pos;
	float d;

	bool spawnShards;
	bool live;

	float health;
    
    Keystates keys;
    
    Player *player;
    
    int weaponCooldown;

};

class Projectile {
public:

    void tick();
    void render() const;

	void addCollision( Collider *collider ) const;

	void impact(Tank *target, const vector<pair<float, Tank *> > &adjacency);

    void genEffects( vector< GfxEffects > *gfxe, Coord2 pos ) const;

    Projectile();   // does not start in usable state
    Projectile(const Coord2 &pos, float d, const IDBProjectile *projtype, Tank *owner);

    bool live;

private:
    
    void dealDamage(float dmg, Tank *target);
    
    Coord2 movement() const;

    Coord2 nexttail() const;

    // Missile velocity is a factor of three things:
    // (1) Acceleration - constantly increases
    // (2) Backdrop - a constant value
    // (3) Sidedrop - approaches a constant offset
    Coord2 missile_accel() const;
    Coord2 missile_backdrop() const;
    Coord2 missile_sidedrop() const;
    float missile_sidedist;

	Coord2 pos;
	float d;

    Coord2 lasttail;

    int age;
    
    const IDBProjectile *projtype;
    Tank *owner;

};

class Game {
public:

	bool runTick( const vector< Keystates > &keys );
    void ai(const vector<Ai *> &ais) const;
    void renderToScreen( int player ) const;

    float firepowerSpent;

    Game();
	Game(vector<Player> *playerdata, const Level &level);

private:
    
    vector<pair<float, Tank *> > genTankDistance(const Coord2 &center);

	int frameNm;
    int framesSinceOneLeft;

	vector< Tank > players;
	vector< vector< Projectile > > projectiles;
	vector< GfxEffects > gfxeffects;
	Gamemap gamemap;

	Collider collider;

};

#endif
