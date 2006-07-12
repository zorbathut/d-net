#ifndef DNET_GAME
#define DNET_GAME

#include "collide.h"
#include "gamemap.h"
#include "input.h"
#include "itemdb.h"
#include "game_effects.h"

using namespace std;

class Collider;
class GameAi;
  
class Tank;
class Projectile;

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

  void addCollision(Collider *collider, const Keystates &kst) const;

  Tank();

  vector<Coord4> getCurrentCollide() const;
  vector<Coord4> getNextCollide(const Keystates &keys) const;

  vector<Coord2> getTankVertices(Coord2 pos, float td) const;
  Coord2 getFiringPoint() const;

  pair<float, float> getNextInertia(const Keystates &keys) const;
  pair<Coord2, float> getNextPosition(const Keystates &keys) const;

  bool takeDamage(float amount); // returns true on kill
  void genEffects(vector<smart_ptr<GfxEffects> > *gfxe, vector<Projectile> *projectiles, const vector<pair<float, Tank *> > &adjacency, Gamemap *gm);
  
  bool initted;

  Team *team;
  
  int zone_current;
  int zone_frames;

  Coord2 pos;
  float d;
  pair<float, float> inertia;

  bool spawnShards;
  bool live;

  float health;
  
  Keystates keys;
  
  Player *player;
  
  int weaponCooldown;
  float weaponCooldownSubvals[SIMUL_WEAPONS];

  // these exist for the DPS calculations
  int framesSinceDamage;
  
  // this exists for the DPH calculations
  float damageTakenPreviousHits;
  
  // this exists for both :D
  float damageTaken;

};

class Projectile {
public:

  void tick(vector<smart_ptr<GfxEffects> > *gfx);
  void render() const;

  void addCollision(Collider *collider) const;

  void impact(Coord2 pos, Tank *target, const vector<pair<float, Tank *> > &adjacency, vector<smart_ptr<GfxEffects> > *gfxe, Gamemap *gm);

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

enum {GMODE_STANDARD, GMODE_CHOICE, GMODE_TEST, GMODE_DEMO, GMODE_LAST};
enum {FACTION_NULL, FACTION_SMALL, FACTION_MEDIUM, FACTION_BIG, FACTION_LAST};
enum {DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_DPH, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_LAST};

class Game {
public:
  
  void initStandard(vector<Player> *playerdata, const Level &level, vector<const IDBFaction *> *wins);
  void initChoice(vector<Player> *playerdata);
  void initTest(Player *playerdata, const Float4 &bounds);
  void initDemo(vector<Player> *playerdata, float boxradi, const float *xps, const float *yps, const int *modes);

  bool runTick(const vector<Keystates> &keys);
  void ai(const vector<GameAi *> &ais) const;
  void renderToScreen() const;

  int winningTeam() const;
  vector<int> teamBreakdown() const;

  int frameCount() const;

  // used for demos
  Coord2 queryPlayerLocation(int id) const;
  void kill(int id);
  void respawnPlayer(int id, Coord2 pos, float facing);

  void addStatHit();  // used for damage-per-hit calculations

  float firepowerSpent;

  Game();

private:
  
  void initCommon(const vector<Player*> &playerdata, const Level &level);
  
  void addTankStatusText(int tankid, const string &text, float duration);
  vector<pair<float, Tank *> > genTankDistance(const Coord2 &center);

  int frameNm;
  int frameNmToStart;
  bool freezeUntilStart;
  int framesSinceOneLeft;

  vector<Team> teams;

  vector<Tank> tanks;
  vector<BombardmentState> bombards;
  vector<vector<Projectile> > projectiles;
  vector<smart_ptr<GfxEffects> > gfxeffects;
  Gamemap gamemap;

  Collider collider;

  vector<pair<vector<Coord2>, Color> > zones;

  Float2 zoom_center;
  Float2 zoom_size;

  Float2 zoom_speed;
  
  int gamemode;
  
  vector<int> demomode_playermodes;
  float demomode_boxradi; // used for the clearscreen and font sizes
  
  int demomode_hits;

  vector<const IDBFaction *> *wins;
  
  Game(const Game &rhs);      // do not implement
  void operator=(const Game &rhs);

};

#endif
