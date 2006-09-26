#ifndef DNET_GAME_UTIL
#define DNET_GAME_UTIL

#include "player.h"
#include "game_effects.h"
#include "collide.h"
#include "gamemap.h"

class Tank;

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
  
  Tank *tank() const { return tank_int; };
  Player *player() const { return player_int; };    // This is just a pointer class, there's no such thing as CTPP currently.
  
  operator void*() { CHECK(!tank_int == !player_int); return (void*)tank_int; };
  TPP(Tank *tank, Player *player) : tank_int(tank), player_int(player) { CHECK(!tank_int == !player_int); };
};


class GameImpactContext {
public:
  const vector<TPP> *players;
  vector<smart_ptr<GfxEffects> > *effects;
  Gamemap *gamemap;

  vector<pair<float, Tank *> > getAdjacency(const Coord2 &pos) const;

  GameImpactContext(const vector<TPP> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap) : players(players), effects(effects), gamemap(gamemap) { };
};

class Projectile {
public:

  void tick(vector<smart_ptr<GfxEffects> > *gfx);
  void render(const vector<Coord2> &tposes) const;

  void addCollision(Collider *collider) const;
  Coord2 warheadposition() const;

  void impact(Coord2 pos, TPP target, const GameImpactContext &gic);

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

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Tank *impact, Tank *owner, const GameImpactContext &gic, float damagecredit, bool killcredit);

#endif
