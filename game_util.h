#ifndef DNET_GAME_UTIL
#define DNET_GAME_UTIL

#include "game_effects.h"
#include "gamemap.h"
#include "itemdb.h"

using namespace std;

class Tank;

class Team {
public:
  bool weapons_enabled;
  Color color;
  bool swap_colors;

  Team();
};

class GameImpactContext {
public:
  const vector<Tank *> players;
  vector<smart_ptr<GfxEffects> > *effects;
  Gamemap *gamemap;

  vector<pair<float, Tank *> > getAdjacency(const Coord2 &pos) const;

  GameImpactContext(vector<Tank> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap);
};

class LauncherLocation {
  const Tank *tank_int; // if null, not tank
  
  Coord2 loc_int;
  float d_int;
public:
  
  bool isTank() const;

  const Tank &tank() const;

  Coord2 loc() const;
  float d() const;

  LauncherLocation(const Tank *tank);
};

class ProjectilePack;
void launchProjectile(const IDBLauncherAdjust &launcher, const LauncherLocation &location, ProjectilePack *projpack, int owner, const GameImpactContext &gic);
void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Tank *impact, Tank *owner, const GameImpactContext &gic, float damagecredit, bool killcredit);

#endif
