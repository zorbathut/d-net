#ifndef DNET_GAME_UTIL
#define DNET_GAME_UTIL

#include "game_effects.h"
#include "gamemap.h"
#include "recorder.h"

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
  Rng *rng;

  Recorder *recorder;

  // This is kind of flawed because it actually does modify Recorder.
  void record(const IDBWarheadAdjust &warhead, Coord2 pos, const Tank *impact_tank, const Tank *owner_tank) const;

  vector<pair<Coord, Tank *> > getAdjacency(const Coord2 &pos) const;
  int getClosestFoeId(const Coord2 &pos, int owner) const;
  Coord getClosestFoeDistance(const Coord2 &pos, int owner) const;

  GameImpactContext(vector<Tank> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap, Rng *rng, Recorder *recorder);

private:
  int findTankId(const Tank *tank) const;
};

class ProjectilePack;
class GamePlayerContext {
public:
  ProjectilePack *projpack;
  Tank *owner;

  const GameImpactContext *gic;

  int owner_id() const;

  GamePlayerContext(Tank *owner, ProjectilePack *projpack, const GameImpactContext &gic);
};

class DeployLocation {
  const Tank *tank_int; // if null, not tank
  
  Coord2 pos_int;
  Coord d_int;
  
  bool has_impacted_ang;
  Coord impacted_ang_int;
public:
  
  bool isTank() const;

  const Tank &tank() const;
  Coord vengeance() const;

  Coord2 pos() const;
  Coord d() const;
  Coord impacted_ang() const;

  DeployLocation(const Tank *tank);
  DeployLocation(Coord2 pos, Coord d);
  DeployLocation(Coord2 pos, Coord d, Coord impacted_ang);
};

struct DamageFlags {
  Coord damagecredit;
  bool killcredit;
  bool glory;
  
  DamageFlags(Coord damagecredit, bool killcredit, bool glory) : damagecredit(damagecredit), killcredit(killcredit), glory(glory) { };
};

void deployProjectile(const IDBDeployAdjust &deploy, const DeployLocation &location, const GamePlayerContext &gpc, const DamageFlags &flags, vector<float> *ang = NULL);

const Coord NO_NORMAL = Coord(-200000000);
void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Coord normal, Coord2 vel, Tank *impact, const GamePlayerContext &gpc, const DamageFlags &flags, bool impacted);
void detonateWarheadDamageOnly(const IDBWarheadAdjust &warhead, Tank *impact, const vector<pair<float, Tank*> > &radius);
void detonateBombardment(const IDBBombardmentAdjust &bombard, Coord2 pos, Coord direction, const GamePlayerContext &gpc);

void triggerInstant(const IDBInstantAdjust &instant, Coord2 pos, const GamePlayerContext &gpc, const DamageFlags &flags);

void adler(Adler32 *adl, const Team &team);
void adler(Adler32 *adl, const DamageFlags &df);

#endif
