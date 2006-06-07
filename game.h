#ifndef DNET_GAME
#define DNET_GAME

#include <vector>

using namespace std;

#include "gamemap.h"
#include "player.h"
#include "input.h"
#include "collide.h"

class Collider;
class Ai;
  
class Tank;
class Projectile;

// there's a lot of redundancy here, but ATM I don't care
// I'm not really sure whether inheritance will honestly be better thanks to allocation overhead, though
class GfxEffects {
public:

  void move();
  void render() const;
  bool dead() const;

  int type;
  enum {EFFECT_POINT, EFFECT_LINE, EFFECT_CIRCLE, EFFECT_TEXT, EFFECT_PATH, EFFECT_PING};
  
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
  
  Float2 ping_pos;
  float ping_radius_d;
  float ping_thickness_d;

  GfxEffects();

};

class Team {
public:
  bool weapons_enabled;
  Color color;
  bool swap_colors;

  Team();
};

class Tank {
public:
  
  void init(Player *player);
  
  void tick(const Keystates &kst);
  void render() const;

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

  Team *team;
  
  int zone_current;
  int zone_frames;

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

  void tick(vector<GfxEffects> *gfx);
  void render() const;

  void addCollision( Collider *collider ) const;

  void impact(Coord2 pos, Tank *target, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm);

  bool isLive() const;

  Projectile();   // does not start in usable state
  Projectile(const Coord2 &pos, float d, const IDBProjectileAdjust &projtype, Tank *owner);

private:
  
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
  
  IDBProjectileAdjust projtype;
  Tank *owner;
  
  bool live;

};

class BombardmentState {
public:
  enum { BS_OFF, BS_SPAWNING, BS_ACTIVE, BS_FIRING, BS_COOLDOWN, BS_LAST };
  Coord2 loc;
  int timer;
  int state;
  
  BombardmentState() {
    loc = Coord2(0, 0);
    timer = 0;
    state = BS_OFF;
  }
};

enum {GMODE_STANDARD, GMODE_CHOICE, GMODE_DEMO, GMODE_LAST};
enum {FACTION_NULL, FACTION_SMALL, FACTION_MEDIUM, FACTION_BIG, FACTION_LAST};

class Game {
public:
  
  void initStandard(vector<Player> *playerdata, const Level &level, vector<const IDBFaction *> *wins, int factionmode);
  void initChoice(vector<Player> *playerdata);
  void initDemo(Player *playerdata, const Float4 &bounds);

  bool runTick( const vector< Keystates > &keys );
  void ai(const vector<Ai *> &ais) const;
  void renderToScreen() const;

  int winningTeam() const;
  vector<int> teamBreakdown() const;

  float firepowerSpent;

  Game();

private:
  
  void initCommon(const vector<Player*> &playerdata, const Level &level, int factionmode);
  
  void addTankStatusText(int tankid, const string &text, float duration);
  vector<pair<float, Tank *> > genTankDistance(const Coord2 &center);

  int frameNm;
  int frameNmToStart;
  bool freezeUntilStart;
  int framesSinceOneLeft;

  vector<Team> teams;

  vector<Tank> players;
  vector<BombardmentState> bombards;
  vector<vector<Projectile> > projectiles;
  vector<GfxEffects> gfxeffects;
  Gamemap gamemap;

  Collider collider;

  vector<pair<vector<Coord2>, Color> > zones;

  Float2 zoom_center;
  Float2 zoom_size;

  Float2 zoom_speed;
  
  int gamemode;

  vector<const IDBFaction *> *wins;
  
  Game(const Game &rhs);      // do not implement
  void operator=(const Game &rhs);

};

#endif
