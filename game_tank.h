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

  void checksum(Adler32 *adl) const;

  // Collision system
  void addCollision(Collider *collider, int owner) const;

  vector<Coord4> getCurrentCollide() const;
  vector<Coord4> getNextCollide() const;

  vector<Coord2> getTankVertices(const CPosInfo &cpi) const;
  Coord2 getFiringPoint() const;
  Coord2 getRearFiringPoint() const;
  Coord2 getMinePoint(Rng *rng) const;

  CPosInfo getNextPosition() const;

  // Modifiers
  bool takeDamage(Coord amount); // returns true on kill
  void genEffects(const GameImpactContext &gic, ProjectilePack *projectiles, const Player *player);
  void respawn(Coord2 pos, Coord facing);
  void tryToFire(Button keys[SIMUL_WEAPONS], Player *player, ProjectilePack *projectiles, int id, const GameImpactContext &gic, vector<pair<string, float> > *status_text);
  
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
  Coord getGloryResistance() const;
  void addDamage(Coord amount);
  void addKill();
  
  void addAccumulatedScores(Player *player);
  
  // Introspection
  DeployLocation launchData() const;
  Color getColor() const;
  
  Coord getHealth() const;
  bool isLive() const;
  
  // Publics >:(
  
  int team;
  
  int zone_current;
  int zone_frames;

  CPosInfo pi;
  pair<Coord2, Coord> inertia;
  
  Keystates keys;

private:
  pair<Coord2, Coord> getNextInertia(const Keystates &keys) const;

  int weaponCooldown;
  Coord weaponCooldownSubvals[SIMUL_WEAPONS];

  Coord health;
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

  Coord damageDealt;
  int kills;

  Coord glory_resistance;
  int glory_resist_boost_frames;
  
  Coord2 worldFromLocal(const Coord2 &loc) const;

};

inline void adler(Adler32 *adler, const Tank &tank) { tank.checksum(adler); }

#endif
