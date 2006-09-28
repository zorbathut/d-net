#ifndef DNET_GAME_PROJECTILE
#define DNET_GAME_PROJECTILE

#include "game_effects.h"
#include "coord.h"
#include "game_util.h"

class Collider;
class Tank;

class Projectile {
public:

  void tick(vector<smart_ptr<GfxEffects> > *gfx);
  void render(const vector<Coord2> &tposes) const;

  void firstCollide(Collider *collider, int id) const;
  void addCollision(Collider *collider, int id) const;
  void collideCleanup(Collider *collider, int id) const;

  Coord2 warheadposition() const;

  void impact(Coord2 pos, Tank *target, const GameImpactContext &gic);

  bool isLive() const;
  bool isDetonating() const;

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
  bool detonating;
};

class ProjectilePack {
public:
  
  Projectile &find(int id);
  void add(const Projectile &proj);

  void updateCollisions(Collider *collider, int owner);
  void tick(vector<smart_ptr<GfxEffects> > *gfxe, Collider *collider, const GameImpactContext &gic);
  void render(const vector<Coord2> &tankpos) const;

private:
  map<int, Projectile> projectiles;
  vector<int> aid;

  vector<int> newitems;
  vector<int> cleanup;
};

#endif
