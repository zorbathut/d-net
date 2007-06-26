#ifndef DNET_GAME_TANK
#define DNET_GAME_TANK

#include "game_projectile.h"
#include "input.h"
#include "player.h"

using namespace std;

class Tank {
public:
  
  // Construction
  void init(IDBTankAdjust tank, Color color);
  Tank();
  
  // Standard behavior
  void updateInertia(const Keystates &kst);
  void tick();
  void render(const vector<Team> &teams) const;

  // Collision system
  void addCollision(Collider *collider, int owner) const;

  vector<Coord4> getCurrentCollide() const;
  vector<Coord4> getNextCollide() const;

  vector<Coord2> getTankVertices(Coord2 pos, Coord td) const;
  Coord2 getFiringPoint() const;
  Coord2 getRearFiringPoint() const;
  Coord2 getMinePoint(Rng *rng) const;

  pair<Coord2, Coord> getNextPosition() const;

  // Modifiers
  bool takeDamage(float amount); // returns true on kill
  void genEffects(const GameImpactContext &gic, ProjectilePack *projectiles, const Player *player);
  void respawn(Coord2 pos, Coord facing);
  void tryToFire(Button keys[SIMUL_WEAPONS], Player *player, ProjectilePack *projectiles, int id, const GameImpactContext &gic, vector<pair<string, float> > *status_text, float *firepowerSpent);
  
  // For demos and such
  void megaboostHealth();
  void setDead();

  // Statistics
  float getDPS() const;
  float getDPH() const;
  float getDPC(int cycles) const;
  int dumpDamageframes() const;
  void insertDamageframes(int dfr);
  
  bool hasTakenDamage() const;
  void addCycle();
  
  // Damage and kill-related
  float getGloryResistance() const;
  void addDamage(float amount);
  void addKill();
  
  void addAccumulatedScores(Player *player);
  
  // Introspection
  DeployLocation launchData() const;
  Color getColor() const;
  
  float getHealth() const;
  bool isLive() const;
  
  // Publics >:(
  
  int team;
  
  int zone_current;
  int zone_frames;

  Coord2 pos;
  Coord d;
  pair<Coord2, Coord> inertia;
  
  Keystates keys;

private:
  pair<Coord2, Coord> getNextInertia(const Keystates &keys) const;

  int weaponCooldown;
  float weaponCooldownSubvals[SIMUL_WEAPONS];

  float health;
  bool spawnShards;
  bool live;

  IDBTankAdjust tank;
  Color color;

  // this exists for the DPS calculations
  int framesSinceDamage;
  int prerollFrames;
  
  // this exists for the DPC calculations
  float damageTakenPreviousHits;
  
  // this exists for the DPH calculations
  int damageEvents;
  
  // this exists for all :D
  float damageTaken;

  float damageDealt;
  int kills;

  float glory_resistance;
  int glory_resist_boost_frames;
  
  Coord2 worldFromLocal(const Coord2 &loc) const;

};

#endif
