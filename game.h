#ifndef DNET_GAME
#define DNET_GAME

#include "gamemap.h"
#include "gfx.h"
#include "itemdb.h"

#include <vector>
using namespace std;

class Collider;

#define RENDERTARGET_SPECTATOR -1

class Player {
public:

    float maxHealth;
    float turnSpeed;
    float maxSpeed;

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
    void move( float time );
	//bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;

	Tank();

	vector< float > getTankVertices( float tx, float ty, float td ) const;
	pair< float, float > getFiringPoint() const;

    pair< pair< float, float >, float > getDeltaAfterMovement( const Keystates &keys, float x, float y, float d, float t ) const;

	bool takeDamage( float amount ); // returns true on kill
	void genEffects( vector< GfxEffects > *gfxe );
    
    bool initted;
	
	float x;
	float y;
	float d;

	bool spawnShards;
	bool live;

	float health;
    
    float timeLeft;
    Keystates keys;
    
    Player *player;
    
    int weaponCooldown;

};

class Projectile {
public:

	void render() const;

    void startNewMoveCycle();
    void move();
    void move( float time );
	//bool colliding( const Collider &collider ) const;
	void addCollision( Collider *collider ) const;

	void impact( Tank *target );

    void genEffects( vector< GfxEffects > *gfxe ) const;

	float x;
	float y;
	float d;

    float v;
    float damage;

    float timeLeft;

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
