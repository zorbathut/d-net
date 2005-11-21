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
    
class Tank;
class Projectile;

class Player {
public:

    float maxHealth;
    float turnSpeed;
    Coord maxSpeed;

    vector<const IDBUpgrade *> upgrades;
    const IDBWeapon *weapon;
    int shotsLeft;

    const IDBGlory *glory;

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

// there's a lot of redundancy here, but ATM I don't care
// I'm not really sure whether inheritance will honestly be better thanks to allocation overhead, though
class GfxEffects {
public:

	void move();
	void render() const;
	bool dead() const;

    int type;
    enum { EFFECT_POINT, EFFECT_LINE, EFFECT_CIRCLE, EFFECT_TEXT, EFFECT_PATH };
    
    int life;
    int age;
    Color color;
    
    Float2 point_pos;
    Float2 point_vel;
    
    Float2 circle_center;
    float circle_radius;
    
	Float4 line_pos;
	Float4 line_vel;

    Float2 text_pos;
    Float2 text_vel;
    float text_size;
    string text_data;
    
    vector<Float2> path_path;
    Float2 path_pos_start;
    Float2 path_pos_vel;
    Float2 path_pos_acc;
    float path_ang_start;
    float path_ang_vel;
    float path_ang_acc;

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
	void genEffects(vector< GfxEffects > *gfxe, vector<Projectile> *projectiles);
    
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

	void impact(Coord2 pos, Tank *target, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm);

    bool isLive() const;

    Projectile();   // does not start in usable state
    Projectile(const Coord2 &pos, float d, const IDBProjectile *projtype, Tank *owner);

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

    float airbrake_liveness() const;
    float airbrake_velocity;

	Coord2 pos;
	float d;

    Coord2 lasttail;

    int age;
    
    const IDBProjectile *projtype;
    Tank *owner;
    
    bool live;

};

class Game {
public:

	bool runTick( const vector< Keystates > &keys );
    void ai(const vector<Ai *> &ais) const;
    void renderToScreen() const;

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

    vector<char> tankHighlight;

    Float2 zoom_center;
    Float2 zoom_size;

    Float2 zoom_speed;

};

#endif
