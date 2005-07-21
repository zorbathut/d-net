#ifndef DNET_GAME
#define DNET_GAME

#include "gamemap.h"

#include <vector>
using namespace std;

class Collider;

#define RENDERTARGET_SPECTATOR -1

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

	void takeDamage( int amount );
	void genEffects( vector< GfxEffects > *gfxe );
	
	float x;
	float y;
	float d;

	bool spawnShards;
	bool live;

	int health;
    
    float timeLeft;
    Keystates keys;

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

    float timeLeft;

    bool live;

    Projectile();

};

class Game {
public:

	void renderToScreen( int player ) const;
	bool runTick( const vector< Keystates > &keys );

	Game();

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
