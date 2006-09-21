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

class TPP {
private:
  Tank *tank_int;
  Player *player_int;

public:
  
  Tank *tank() { return tank_int; };
  Player *player() { return player_int; };
  
  operator void*() { CHECK(!tank_int == !player_int); return (void*)tank_int; };
  TPP(Tank *tank, Player *player) : tank_int(tank), player_int(player) { CHECK(!tank_int == !player_int); };
};

class Tank {
public:
  
  void init(IDBTankAdjust tank, Color color);
  
  void tick(const Keystates &kst);
  void render(const vector<Team> &teams) const;

  void addCollision(Collider *collider, const Keystates &kst) const;

  Tank();

  vector<Coord4> getCurrentCollide() const;
  vector<Coord4> getNextCollide(const Keystates &keys) const;

  vector<Coord2> getTankVertices(Coord2 pos, float td) const;
  Coord2 getFiringPoint() const;

  pair<float, float> getNextInertia(const Keystates &keys) const;
  pair<Coord2, float> getNextPosition(const Keystates &keys) const;

  bool takeDamage(float amount); // returns true on kill
  void genEffects(vector<smart_ptr<GfxEffects> > *gfxe, vector<Projectile> *projectiles, const vector<pair<float, TPP> > &adjacency, Gamemap *gm, Player *player, int id);

  IDBTankAdjust tank;
  Color color;
  
  int team;
  
  int zone_current;
  int zone_frames;

  Coord2 pos;
  float d;
  pair<float, float> inertia;

  bool spawnShards;
  bool live;

  float health;
  
  Keystates keys;
  
  int weaponCooldown;
  float weaponCooldownSubvals[SIMUL_WEAPONS];

  // this exists for the DPS calculations
  int framesSinceDamage;
  
  // this exists for the DPC calculations
  float damageTakenPreviousHits;
  
  // this exists for the DPH calculations
  int damageEvents;
  
  // this exists for all :D
  float damageTaken;

};

class Projectile {
public:

  void tick(vector<smart_ptr<GfxEffects> > *gfx);
  void render(const vector<Coord2> &tposes) const;

  void addCollision(Collider *collider) const;

  void impact(Coord2 pos, TPP target, const vector<pair<float, TPP> > &adjacency, vector<smart_ptr<GfxEffects> > *gfxe, Gamemap *gm, const vector<TPP> &players);

  bool isLive() const;

  Projectile();   // does not start in usable state
  Projectile(const Coord2 &pos, float d, const IDBProjectileAdjust &projtype, int in_owner);

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

  vector<Coord2> mine_polys() const;
  float mine_facing;

  Coord2 pos;
  float d;

  Coord2 lasttail;

  int age;
  
  IDBProjectileAdjust projtype;
  int owner;
  
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

enum {GMODE_STANDARD, GMODE_CHOICE, GMODE_TEST, GMODE_DEMO, GMODE_CENTERED_DEMO, GMODE_LAST};
enum {FACTION_NULL, FACTION_SMALL, FACTION_MEDIUM, FACTION_BIG, FACTION_LAST};
enum {DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_DPH, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_LAST};

class Game {
public:
  
  void initStandard(vector<Player> *playerdata, const Level &level, vector<const IDBFaction *> *wins);
  void initChoice(vector<Player> *playerdata);
  void initTest(Player *playerdata, const Float4 &bounds);
  void initDemo(vector<Player> *playerdata, float boxradi, const float *xps, const float *yps, const float *facing, const int *modes);
  void initCenteredDemo(Player *playerdata, float zoom);

  bool runTick(const vector<Keystates> &keys, const vector<Player *> &players);
  void ai(const vector<GameAi *> &ais) const;
  void renderToScreen(const vector<const Player *> &players) const;

  int winningTeam() const;
  vector<int> teamBreakdown() const;

  int frameCount() const;

  // used for demos
  Coord2 queryPlayerLocation(int id) const;
  void kill(int id);
  void respawnPlayer(int id, Coord2 pos, float facing);

  void addStatCycle();  // used for damage-per-cycle calculations

  float firepowerSpent;

  Game();

private:
  
  void initCommon(const vector<Player*> &playerdata, const Level &level);
  
  void addTankStatusText(int tankid, const string &text, float duration);
  vector<pair<float, TPP> > genTankDistance(const Coord2 &center, const vector<Player *> &players);

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
  
  float centereddemo_zoom;
  
  int demomode_hits;
  
  float bombardment_tier;
  float getBombardmentIncreasePerSec() const;
  float getTimeUntilBombardmentUpgrade() const;

  vector<const IDBFaction *> *wins;
  
  Game(const Game &rhs);      // do not implement
  void operator=(const Game &rhs);

};

class GamePackage {
public:
  vector<Player> players;
  Game game;

  bool runTick(const vector<Keystates> &keys);
  void renderToScreen() const;
};

#endif
