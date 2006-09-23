#ifndef DNET_GAME_TANK
#define DNET_GAME_TANK

#include "game_util.h"
#include "input.h"

class Tank {
public:
  
  void init(IDBTankAdjust tank, Color color);
  
  void tick(const Keystates &kst);
  void render(const vector<Team> &teams) const;

  void addCollision(Collider *collider, const Keystates &kst) const;

  vector<Coord4> getCurrentCollide() const;
  vector<Coord4> getNextCollide(const Keystates &keys) const;

  vector<Coord2> getTankVertices(Coord2 pos, float td) const;
  Coord2 getFiringPoint() const;

  pair<float, float> getNextInertia(const Keystates &keys) const;
  pair<Coord2, float> getNextPosition(const Keystates &keys) const;

  bool takeDamage(float amount); // returns true on kill
  void genEffects(vector<smart_ptr<GfxEffects> > *gfxe, vector<Projectile> *projectiles, const vector<pair<float, TPP> > &adjacency, Gamemap *gm, Player *player, int id);

  float getDPS() const;
  float getDPH() const;
  float getDPC(int cycles) const;
  
  bool hasTakenDamage() const;
  void addCycle();

  Tank();

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

private:
  // this exists for the DPS calculations
  int framesSinceDamage;
  
  // this exists for the DPC calculations
  float damageTakenPreviousHits;
  
  // this exists for the DPH calculations
  int damageEvents;
  
  // this exists for all :D
  float damageTaken;

};

#endif
