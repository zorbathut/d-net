
#include "itemdb.h"

#include "rng.h"

using namespace std;

template<typename T> vector<typename IDBItemProperties<T>::adjusted> adjust_vector(const vector<const T *> &orig, const IDBAdjustment &adjust) {
  vector<typename IDBItemProperties<T>::adjusted> rv;
  for(int i = 0; i < orig.size(); i++)
    rv.push_back(typename IDBItemProperties<T>::adjusted(orig[i], adjust));
  return rv;
}

/*************
 * IDBDeployAdjust
 */
 
int IDBDeployAdjust::type() const { return idb->type; };

int IDBDeployAdjust::exp_minsplits() const { return idb->exp_minsplits; };
int IDBDeployAdjust::exp_maxsplits() const { return idb->exp_maxsplits; };

int IDBDeployAdjust::exp_minsplitsize() const { return idb->exp_minsplitsize; };
int IDBDeployAdjust::exp_maxsplitsize() const { return idb->exp_maxsplitsize; };

int IDBDeployAdjust::exp_shotspersplit() const { return idb->exp_shotspersplit; };

float IDBDeployAdjust::anglestddev() const { return idb->anglestddev; };

vector<IDBDeployAdjust> IDBDeployAdjust::chain_deploy() const { return adjust_vector(idb->chain_deploy, adjust); }
vector<IDBProjectileAdjust> IDBDeployAdjust::chain_projectile() const { return adjust_vector(idb->chain_projectile, adjust); }
vector<IDBWarheadAdjust> IDBDeployAdjust::chain_warhead() const { return adjust_vector(idb->chain_warhead, adjust); }

float IDBDeployAdjust::stats_damagePerShot() const {
  float mult;
  if(idb->type == DT_NORMAL || idb->type == DT_FORWARD || idb->type == DT_CENTROID || idb->type == DT_MINEPATH || idb->type == DT_REAR) {
    mult = 1.0;
  } else if(idb->type == DT_EXPLODE) {
    mult = (exp_minsplits() + exp_maxsplits()) / 2.0 * exp_shotspersplit();
  } else {
    CHECK(0);
  }

  float val = 0;
  
  vector<IDBDeployAdjust> idbda = chain_deploy();
  for(int i = 0; i < idbda.size(); i++)
    val += idbda[i].stats_damagePerShot();
  
  vector<IDBProjectileAdjust> idbpa = chain_projectile();
  for(int i = 0; i < idbpa.size(); i++)
    val += idbpa[i].stats_damagePerShot();
  
  vector<IDBWarheadAdjust> idbwa = chain_warhead();
  for(int i = 0; i < idbwa.size(); i++)
    val += idbwa[i].stats_damagePerShot();
  
  return val * mult;
}

IDBDeployAdjust::IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWarheadAdjust
 */

float IDBWarheadAdjust::accumulate(const float *damage) const {
  float acc = 0;
  for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
    acc += damage[i] * adjust.adjustmentfactor(i);
  return acc;
}

float IDBWarheadAdjust::impactdamage() const { return accumulate(idb->impactdamage) * mf; }

float IDBWarheadAdjust::radiusdamage() const { return accumulate(idb->radiusdamage) * mf; }
float IDBWarheadAdjust::radiusfalloff() const { return adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->radiusfalloff; }

float IDBWarheadAdjust::wallremovalradius() const { return adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->wallremovalradius; }  // just 'cause :)
float IDBWarheadAdjust::wallremovalchance() const { return idb->wallremovalchance; }
Color IDBWarheadAdjust::radiuscolor_bright() const { return idb->radiuscolor_bright * mf; }
Color IDBWarheadAdjust::radiuscolor_dim() const { return idb->radiuscolor_dim * mf; }

vector<IDBEffectsAdjust> IDBWarheadAdjust::effects_impact() const {
  vector<IDBEffectsAdjust> rv;
  for(int i = 0; i < idb->effects_impact.size(); i++)
    rv.push_back(IDBEffectsAdjust(idb->effects_impact[i], adjust));
  return rv;
}

vector<IDBDeployAdjust> IDBWarheadAdjust::deploy() const {
  vector<IDBDeployAdjust> rv;
  for(int i = 0; i < idb->deploy.size(); i++)
    rv.push_back(IDBDeployAdjust(idb->deploy[i], adjust));
  return rv;
}

float IDBWarheadAdjust::stats_damagePerShot() const {
  float count = impactdamage() + radiusdamage();
  vector<IDBDeployAdjust> dp = deploy();
  for(int i = 0; i < dp.size(); i++) {
    count += dp[i].stats_damagePerShot();
  }
  return count;
}

float IDBWarheadAdjust::stats_damagePerShotType(int type) const {
    // Sometimes I'm clever.
  IDBAdjustment adj = adjust;
  for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
    if(i != type)
      adj.adjusts[i] = -100;
  return IDBWarheadAdjust(idb, adj).stats_damagePerShot();
}

const IDBWarhead *IDBWarheadAdjust::base() const {
  return idb;
}
float IDBWarheadAdjust::multfactor() const {
  return mf;
}
IDBWarheadAdjust IDBWarheadAdjust::multiply(float mult) const {
  return IDBWarheadAdjust(idb, adjust, mf * mult);
}

IDBWarheadAdjust::IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment &in_adjust, float in_multfactor) { idb = in_idb; adjust = in_adjust; mf = in_multfactor; }

/*************
 * IDBProjectileAdjust
 */

int IDBProjectileAdjust::motion() const { return idb->motion; };
float IDBProjectileAdjust::velocity() const { return idb->velocity; };
float IDBProjectileAdjust::length() const { return idb->length; };
float IDBProjectileAdjust::radius_physical() const { return idb->radius_physical; };
float IDBProjectileAdjust::toughness() const { return idb->toughness; };

vector<IDBWarheadAdjust> IDBProjectileAdjust::chain_warhead(float multfactor) const {
  vector<IDBWarheadAdjust> rv;
  for(int i = 0; i < idb->chain_warhead.size(); i++)
    rv.push_back(IDBWarheadAdjust(idb->chain_warhead[i], adjust, multfactor));
  return rv;
}

Color IDBProjectileAdjust::color() const { return idb->color; };
float IDBProjectileAdjust::thickness_visual() const { return idb->thickness_visual; };
int IDBProjectileAdjust::mine_spikes() const { return idb->mine_spikes; };

float IDBProjectileAdjust::halflife() const { return idb->halflife; };
float IDBProjectileAdjust::airbrake_life() const { CHECK(idb->motion == PM_AIRBRAKE); return idb->airbrake_life; }

float IDBProjectileAdjust::missile_stabstart() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_stabstart; }
float IDBProjectileAdjust::missile_stabilization() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_stabilization; }
float IDBProjectileAdjust::missile_sidelaunch() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_sidelaunch; }
float IDBProjectileAdjust::missile_backlaunch() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_backlaunch; }

float IDBProjectileAdjust::dps_duration() const { CHECK(idb->motion == PM_DPS); return idb->dps_duration; }

float IDBProjectileAdjust::stats_damagePerShot() const {
  float val = 0;
  vector<IDBWarheadAdjust> idbwa = chain_warhead();
  for(int i = 0; i < idbwa.size(); i++)
    val += idbwa[i].stats_damagePerShot();
  return val;
}

IDBProjectileAdjust::IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBLauncherAdjust
 */

IDBDeployAdjust IDBLauncherAdjust::deploy() const { return IDBDeployAdjust(idb->deploy, adjust); };

float IDBLauncherAdjust::stats_damagePerShot() const { return deploy().stats_damagePerShot(); };

IDBLauncherAdjust::IDBLauncherAdjust(const IDBLauncher *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWeaponAdjust
 */

const string &IDBWeaponAdjust::name() const { return idb->name; };

IDBLauncherAdjust IDBWeaponAdjust::launcher() const { return IDBLauncherAdjust(idb->launcher, adjust); };

int framesForCooldownEngine(float frames_per_shot, Rng *rng) {
  return (int)floor(frames_per_shot) + (rng->frand() < (frames_per_shot - floor(frames_per_shot)));
};

struct FFCETester {
  FFCETester() {
    const float framn = 2;
    const float gole = framn * 1000;
    int tfram = 0;
    Rng rng(unsync().generate_seed());
    for(int i = 0; i < 1000; i++)
      tfram += framesForCooldownEngine(framn, &rng);
    dprintf("tfram test: should be %dish, is %d\n", int(gole), tfram);
    
    CHECK(tfram > gole * .9 && tfram < gole * 1.1);
  }
} test;

int IDBWeaponAdjust::framesForCooldown(Rng *rng) const { 
  return framesForCooldownEngine(FPS / firerate(), rng);
}
float IDBWeaponAdjust::firerate() const {
  return idb->firerate * adjust.adjustmentfactor(IDBAdjustment::TANK_FIRERATE);
}
Money IDBWeaponAdjust::cost(int amount) const { return (cost_pack() * amount + Money(idb->quantity - 1)) / idb->quantity; };
Money IDBWeaponAdjust::cost_pack() const { return idb->base_cost / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_WEAPON); };
Money IDBWeaponAdjust::sellcost(int amount) const { return cost_pack() * adjust.recyclevalue() * amount / idb->quantity; };

bool IDBWeaponAdjust::glory_resistance() const { return idb->glory_resistance; };

float IDBWeaponAdjust::stats_damagePerSecond() const { return launcher().stats_damagePerShot() * firerate(); }
float IDBWeaponAdjust::stats_damagePerSecondType(int type) const {
  // Sometimes I'm clever.
  IDBAdjustment adj = adjust;
  for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
    if(i != type)
      adj.adjusts[i] = -100;
  return IDBWeaponAdjust(idb, adj).stats_damagePerSecond();
}

float IDBWeaponAdjust::stats_costPerDamage() const { return (float)cost_pack().value() / idb->quantity / launcher().stats_damagePerShot(); }
float IDBWeaponAdjust::stats_costPerSecond() const { return (float)cost_pack().value() / idb->quantity * firerate(); }

IDBWeaponAdjust::IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBGloryAdjust
 */

vector<IDBDeployAdjust> IDBGloryAdjust::blast() const {
  vector<IDBDeployAdjust> rv;
  for(int i = 0; i < idb->blast.size(); i++)
    rv.push_back(IDBDeployAdjust(idb->blast[i], adjust));
  return rv;
}
IDBDeployAdjust IDBGloryAdjust::core() const { return IDBDeployAdjust(idb->core, adjust); };

Money IDBGloryAdjust::cost() const { return idb->base_cost; };
Money IDBGloryAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

float IDBGloryAdjust::stats_averageDamage() const {
  float v = core().stats_damagePerShot();
  vector<IDBDeployAdjust> vd = blast();
  for(int i = 0; i < vd.size(); i++)
    v += vd[i].stats_damagePerShot();
  return v;
}

IDBGloryAdjust::IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBUpgradeAdjust
 */

Money IDBUpgradeAdjust::cost() const { return tank->upgrade_base * idb->costmult / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_UPGRADE); };  // yes, it's based off the tank base cost, not the tank adjusted cost
Money IDBUpgradeAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

IDBUpgradeAdjust::IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBTank *in_tank, const IDBAdjustment &in_adjust) { idb = in_idb; tank = in_tank; adjust = in_adjust; };

/*************
 * IDBBombardmentAdjust
 */

float IDBBombardmentAdjust::lockdelay() const { return idb->lockdelay / adjust.adjustmentfactor(IDBAdjustment::BOMBARDMENT_SPEED); };
float IDBBombardmentAdjust::unlockdelay() const { return idb->unlockdelay / adjust.adjustmentfactor(IDBAdjustment::BOMBARDMENT_SPEED); };

bool IDBBombardmentAdjust::showdirection() const {
  return idb->showdirection;
}

vector<IDBWarheadAdjust> IDBBombardmentAdjust::warheads() const {
  CHECK(valid_level);
  vector<IDBWarheadAdjust> rv;
  for(int i = 0; i < idb->warheads.size(); i++)
    rv.push_back(IDBWarheadAdjust(idb->warheads[i], adjust));
  return rv;
}
vector<IDBProjectileAdjust> IDBBombardmentAdjust::projectiles() const {
  CHECK(valid_level);
  vector<IDBProjectileAdjust> rv;
  for(int i = 0; i < idb->projectiles.size(); i++)
    rv.push_back(IDBProjectileAdjust(idb->projectiles[i], adjust));
  return rv;
}
vector<IDBEffectsAdjust> IDBBombardmentAdjust::effects() const { return adjust_vector(idb->effects, adjust); }

Money IDBBombardmentAdjust::cost() const { return idb->cost; };
Money IDBBombardmentAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

float IDBBombardmentAdjust::stats_damagePerShot() const {
  float accum = 0;
  for(int i = 0; i < warheads().size(); i++)
    accum += warheads()[i].stats_damagePerShot();
  for(int i = 0; i < projectiles().size(); i++)
    accum += projectiles()[i].stats_damagePerShot();
  return accum;
}
float IDBBombardmentAdjust::stats_damagePerShotType(int type) const {
  // Sometimes I'm clever.
  IDBAdjustment adj = adjust;
  for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
    if(i != type)
      adj.adjusts[i] = -100;
  return IDBBombardmentAdjust(idb, adj, 0).stats_damagePerShot();
}

IDBBombardmentAdjust::IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment &in_adjust, int in_bombardlevel) {
  idb = in_idb; adjust = in_adjust;
  if(in_bombardlevel != -1) {
    CHECK(in_bombardlevel >= 0);
    valid_level = true;
    for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
      adjust.adjusts[i] += 25 * in_bombardlevel;
    adjust.adjusts[IDBAdjustment::WARHEAD_RADIUS_FALLOFF] += 20 * in_bombardlevel;
    adjust.adjusts[IDBAdjustment::BOMBARDMENT_SPEED] += 25 * in_bombardlevel;
    if(in_bombardlevel > 0)
      adjust.ignore_excessive_radius = true;
  } else {
    valid_level = false;
  }
};

/*************
 * IDBTankAdjust
 */

Money IDBTankAdjust::cost() const { return idb->base_cost / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_TANK); };
Money IDBTankAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

float IDBTankAdjust::maxHealth() const { return idb->health * adjust.adjustmentfactor(IDBAdjustment::TANK_ARMOR); };
float IDBTankAdjust::turnSpeed() const { return idb->handling * adjust.adjustmentfactor(IDBAdjustment::TANK_TURN); };
float IDBTankAdjust::maxSpeed() const { return idb->engine * adjust.adjustmentfactor(IDBAdjustment::TANK_SPEED); };

float IDBTankAdjust::mass() const { return idb->mass; };    // BAM

const vector<Coord2> &IDBTankAdjust::vertices() const { return idb->vertices; };
Coord2 IDBTankAdjust::firepoint() const { return idb->firepoint; };
Coord2 IDBTankAdjust::rearfirepoint() const { return idb->rearfirepoint; };
const vector<Coord2> &IDBTankAdjust::minepath() const { return idb->minepath; };
vector<Coord2> IDBTankAdjust::getTankVertices(Coord2 pos, float td) const { return idb->getTankVertices(pos, td); };

const IDBTank *IDBTankAdjust::base() const { return idb; };

IDBTankAdjust::IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs) {
  return lhs.idb == rhs.idb && lhs.adjust == rhs.adjust;
}

/*************
 * IDBImplantSlotAdjust
 */

Money IDBImplantSlotAdjust::cost() const { return idb->cost / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_IMPLANT); };

IDBImplantSlotAdjust::IDBImplantSlotAdjust(const IDBImplantSlot *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBImplantAdjust
 */

Money IDBImplantAdjust::costToLevel(int curlevel) const {
  // I'm not quite sure what to do here
  Money flev = Money(5000);
  for(int i = 0; i < curlevel; i++)
    flev = flev * 30;
  return flev / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_IMPLANT);
}

IDBImplantAdjust::IDBImplantAdjust(const IDBImplant *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; }

/*************
 * IDBEffectsAdjust
 */

IDBEffects::IDBEType IDBEffectsAdjust::type() const { return idb->type; }
int IDBEffectsAdjust::quantity() const { return idb->quantity; }

float IDBEffectsAdjust::particle_inertia() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_inertia; }
float IDBEffectsAdjust::particle_reflect() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_reflect; }

float IDBEffectsAdjust::particle_spread() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_spread; }

float IDBEffectsAdjust::particle_slowdown() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_slowdown; }
float IDBEffectsAdjust::particle_lifetime() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_lifetime; }

float IDBEffectsAdjust::particle_radius() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_radius; }
Color IDBEffectsAdjust::particle_color() const { CHECK(type() == IDBEffects::EFT_PARTICLE); return idb->particle_color; }

float IDBEffectsAdjust::ionblast_radius() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_radius * adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF); }
float IDBEffectsAdjust::ionblast_duration() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_duration; }

const vector<pair<int, Color> > &IDBEffectsAdjust::ionblast_visuals() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_visuals; }

IDBEffectsAdjust::IDBEffectsAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; }
