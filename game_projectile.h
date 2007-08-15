#ifndef DNET_GAME_PROJECTILE
#define DNET_GAME_PROJECTILE

#include "game_util.h"

using namespace std;

class Collider;
class Tank;

class Projectile {
public:

  void tick(vector<smart_ptr<GfxEffects> > *gfx, const GameImpactContext &gic, int owner);
  void render(const vector<Coord2> &tposes) const;

  void checksum(Adler32 *adl) const;

  void firstCollide(Collider *collider, int owner, int id) const;
  void addCollision(Collider *collider, int owner, int id) const;
  void collideCleanup(Collider *collider, int owner, int id) const;

  Coord2 warheadposition() const;

  void detonate(Coord2 pos, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted);

  bool isLive() const;
  bool isDetonating() const;

  bool isConsumed() const; // !isLive || isDetonating

  float durability() const;

  Projectile();   // does not start in usable state
  Projectile(const Coord2 &pos, Coord d, const IDBProjectileAdjust &projtype, Rng *rng, const DamageFlags &damageflags);

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
  Coord missile_sidedist;

  Coord airbrake_liveness() const;
  Coord airbrake_velocity;

  Coord2 boomerang_abspos;
  Coord boomerang_yfactor;
  Coord boomerang_angle;
  Coord boomerang_lastchange;

  vector<Coord2> star_polys() const;
  Coord star_facing;

  Coord spider_vector;  // spider-vector, spider-vector, doin' things like a spectre. what's he like? no-one knows! he's got radioactive clothes! beware! here comes the spider vector
  enum { SV_NONE = -1000 }; // this seems like a reasonable sentinel

  Coord2 pos;
  Coord d;

  Coord2 lasttail;
  Coord arrow_spin;
  Coord arrow_spin_next;
  bool arrow_spin_parity;

  int age;
  
  IDBProjectileAdjust projtype;
  
  bool live;
  bool detonating;
  
  Coord distance_traveled;
  
  Coord closest_enemy_tank;
  
  DamageFlags damageflags;
};

class ProjectilePack {
public:
  
  Projectile &find(int id);
  void add(const Projectile &proj);

  void updateCollisions(Collider *collider, int owner);
  void tick(vector<smart_ptr<GfxEffects> > *gfxe, Collider *collider, int owner, const GameImpactContext &gic);
  void render(const vector<Coord2> &tankpos) const;
  void checksum(Adler32 *adl) const;

private:
  map<int, Projectile> projectiles;
  vector<int> aid;

  vector<int> newitems;
  vector<int> cleanup;
};

inline void adler(Adler32 *adl, const ProjectilePack &ppk) { ppk.checksum(adl); }
inline void adler(Adler32 *adl, const Projectile &ppk) { ppk.checksum(adl); }

#endif
