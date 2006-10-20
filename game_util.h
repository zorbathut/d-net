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

class DeployLocation {
  const Tank *tank_int; // if null, not tank
  
  Coord2 pos_int;
  float d_int;
public:
  
  bool isTank() const;

  const Tank &tank() const;

  Coord2 pos() const;
  float d() const;

  DeployLocation(const Tank *tank);
  DeployLocation(Coord2 pos, float d);
};

class ProjectilePack;
void deployProjectile(const IDBDeployAdjust &deploy, const DeployLocation &location, ProjectilePack *projpack, int owner, const GameImpactContext &gic);
void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Tank *impact, Tank *owner, const GameImpactContext &gic, float damagecredit, bool killcredit);

#endif
