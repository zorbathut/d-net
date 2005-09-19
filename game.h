#ifndef DNET_GAME
#define DNET_GAME

#include "gamemap.h"
#include "gfx.h"
#include "itemdb.h"
#include "coord.h"

#include <vector>
using namespace std;

class Collider;

#define RENDERTARGET_SPECTATOR -1

class Player {
public:

    float maxHealth;
    float turnSpeed;
    Coord maxSpeed;

    vector<const Upgrade *> upgrades;
    const Weapon *weapon;
    int shotsLeft;

    Color color;
    Dvec2 faction_symb;

    int cash;

    void reCalculate();
    bool hasUpgrade(const Upgrade *upg) const;

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
    
	//void tick();
	void render( int tankid ) const;

    void startNewMoveCycle();
    void setKeys( const Keystates &keystates );
    void move();
    void move( Coord time );
    void moveTo( Coord time );
	//bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;

	Tank();

	vector<Coord2> getTankVertices( Coord tx, Coord ty, float td ) const;
	Coord2 getFiringPoint() const;

    pair<Coord2, float> getDeltaAfterMovement( const Keystates &keys, Coord x, Coord y, float d, Coord t ) const;

	bool takeDamage( float amount ); // returns true on kill
	void genEffects( vector< GfxEffects > *gfxe );
    
    bool initted;
	
	Coord x;
	Coord y;
	float d;

	bool spawnShards;
	bool live;

	float health;
    
    Coord timeDone;
    Keystates keys;
    
    Player *player;
    
    int weaponCooldown;

};

class Projectile {
public:

	void render() const;

    void startNewMoveCycle();
    void move();
    void move( Coord time );
	//bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;

	void impact( Tank *target );

    void genEffects( vector< GfxEffects > *gfxe ) const;

	Coord x;
	Coord y;
	float d;

    Coord v;
    float damage;

    Coord timeLeft;

    bool live;
    
    Tank *owner;

    Projectile();

};

class Game {
public:

	void renderToScreen( int player ) const;
	bool runTick( const vector< Keystates > &keys );

    float firepowerSpent;

    Game();
	Game(vector<Player> *playerdata, const Level &level);

private:

	int frameNm;
    int framesSinceOneLeft;

	vector< Tank > players;
	vector< vector< Projectile > > projectiles;
	vector< GfxEffects > gfxeffects;
	Gamemap gamemap;

	Collider collider;

};

#endif
