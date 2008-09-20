#ifndef DNET_GAME_PROJECTILE
#define DNET_GAME_PROJECTILE

#include "game_util.h"

using namespace std;

class Collider;
class Tank;

class Projectile {
public:

  void tick(const GameImpactContext &gic, int owner);
  void spawnEffects(vector<smart_ptr<GfxEffects> > *gfxe) const;
  void render() const;

  void checksum(Adler32 *adl) const;

  void firstCollide(Collider *collider, int owner, int id) const;
  void addCollision(Collider *collider, int owner, int id) const;
  void collideCleanup(Collider *collider, int owner, int id) const;

  Coord2 warheadposition() const;

  void trigger(Coord t, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted);

  bool isLive() const;
  bool isDetonating() const;

  bool isConsumed() const; // !isLive || isDetonating

  float durability() const;

  Projectile();   // does not start in usable state
  Projectile(const Coord2 &pos, Coord d, const IDBProjectileAdjust &projtype, Rng *rng, const DamageFlags &damageflags);

private:
  
  void triggerstandard(Coord2 pos, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted);

  struct ProjPostState {
    CPosInfo pi;
    Coord distance_traveled;
    
    Coord airbrake_velocity;
    
    Coord arrow_spin;
    
    Coord hunter_vel;
    
    ProjPostState();
  } now, last;
  friend void adler(Adler32 *adl, const ProjPostState &pps);
  
  vector<Coord4> polys(const ProjPostState &stat) const;
  Float2 rearspawn(const ProjPostState &stat) const;

  // Missile velocity is a factor of three things:
  // (1) Acceleration - constantly increases
  // (2) Backdrop - a constant value
  // (3) Sidedrop - approaches a constant offset
  Coord2 missile_accel() const;
  Coord2 missile_backdrop() const;
  Coord2 missile_sidedrop() const;
  Coord missile_sidedist;
  
  Coord2 hk_drift;

  Coord airbrake_liveness() const;

  Coord2 boomerang_abspos;
  Coord boomerang_yfactor;
  Coord boomerang_angle;
  Coord boomerang_lastchange;

  Coord star_facing;

  bool arrow_spin_parity;
  float velocity;

  Coord age;
  
  IDBProjectileAdjust projtype;
  
  bool first;
  bool live;
  bool detonating;
  
  Coord proxy_visibility;
  
  Coord closest_enemy_tank;
  
  DamageFlags damageflags;
};

class ProjectilePack {
public:
  
  Projectile &find(int id);
  void add(const Projectile &proj);

  void updateCollisions(Collider *collider, int owner);
  void tick(Collider *collider, int owner, const GameImpactContext &gic);
  void spawnEffects(vector<smart_ptr<GfxEffects> > *gfxe) const;
  void cleanup(Collider *collider, int owner);
  void render() const;
  void checksum(Adler32 *adl) const;

private:
  map<int, Projectile> projectiles;
  vector<int> aid;

  vector<int> newitems;
};

inline void adler(Adler32 *adl, const ProjectilePack &ppk) { ppk.checksum(adl); }
inline void adler(Adler32 *adl, const Projectile &ppk) { ppk.checksum(adl); }
void adler(Adler32 *adl, const Projectile::ProjPostState &pps);

#endif
