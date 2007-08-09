
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

float IDBDeployAdjust::directed_range() const { return idb->directed_range; };

vector<IDBDeployAdjust> IDBDeployAdjust::chain_deploy() const { return adjust_vector(idb->chain_deploy, adjust); }
vector<IDBProjectileAdjust> IDBDeployAdjust::chain_projectile() const { return adjust_vector(idb->chain_projectile, adjust); }
vector<IDBWarheadAdjust> IDBDeployAdjust::chain_warhead() const { return adjust_vector(idb->chain_warhead, adjust); }

float IDBDeployAdjust::stats_damagePerShot() const {
  float mult;
  if(idb->type == DT_NORMAL || idb->type == DT_FORWARD || idb->type == DT_CENTROID || idb->type == DT_MINEPATH || idb->type == DT_REAR || idb->type == DT_DIRECTED) {
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

void IDBDeployAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
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

Coord IDBWarheadAdjust::impactdamage() const { return accumulate(idb->impactdamage) * mf; }

Coord IDBWarheadAdjust::radiusdamage() const { return accumulate(idb->radiusdamage) * mf; }
Coord IDBWarheadAdjust::radiusfalloff() const { return (adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->radiusfalloff * idb->radiusexplosive) + (idb->radiusfalloff * (1 - idb->radiusexplosive)); }

Coord IDBWarheadAdjust::wallremovalradius() const { return adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->wallremovalradius; }  // just 'cause :)
Coord IDBWarheadAdjust::wallremovalchance() const { return idb->wallremovalchance; }
Color IDBWarheadAdjust::radiuscolor_bright() const { return idb->radiuscolor_bright * mf.toFloat(); }
Color IDBWarheadAdjust::radiuscolor_dim() const { return idb->radiuscolor_dim * mf.toFloat(); }

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
  float count = (impactdamage() + radiusdamage()).toFloat();
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
Coord IDBWarheadAdjust::multfactor() const {
  return mf;
}
IDBWarheadAdjust IDBWarheadAdjust::multiply(float mult) const {
  return IDBWarheadAdjust(idb, adjust, mf * mult);
}

void IDBWarheadAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
  adler(adl, mf);
}
IDBWarheadAdjust::IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment &in_adjust, Coord in_multfactor) { idb = in_idb; adjust = in_adjust; mf = in_multfactor; }

/*************
 * IDBProjectileAdjust
 */

int IDBProjectileAdjust::motion() const { return idb->motion; };
float IDBProjectileAdjust::velocity() const { return idb->velocity; };
float IDBProjectileAdjust::radius_physical() const { return idb->radius_physical; };
float IDBProjectileAdjust::durability() const { return idb->durability; };
float IDBProjectileAdjust::proximity() const { return idb->proximity; };

vector<IDBWarheadAdjust> IDBProjectileAdjust::chain_warhead(float multfactor) const {
  vector<IDBWarheadAdjust> rv;
  for(int i = 0; i < idb->chain_warhead.size(); i++)
    rv.push_back(IDBWarheadAdjust(idb->chain_warhead[i], adjust, multfactor));
  return rv;
}

vector<IDBDeployAdjust> IDBProjectileAdjust::chain_deploy() const { return adjust_vector(idb->chain_deploy, adjust); }
vector<IDBEffectsAdjust> IDBProjectileAdjust::chain_effects() const { return adjust_vector(idb->chain_effects, adjust); }

int IDBProjectileAdjust::shape() const { return idb->shape; }

float IDBProjectileAdjust::defshape_length() const { return idb->defshape_length; };

float IDBProjectileAdjust::arrow_width() const { CHECK(shape() == PS_ARROW); return idb->arrow_width; }
float IDBProjectileAdjust::arrow_height() const { CHECK(shape() == PS_ARROW); return idb->arrow_height; }
float IDBProjectileAdjust::arrow_rotate() const { CHECK(shape() == PS_ARROW); return idb->arrow_rotate; }
float IDBProjectileAdjust::visual_thickness() const { return idb->visual_thickness; };
Color IDBProjectileAdjust::color() const { return idb->color; };

int IDBProjectileAdjust::mine_spikes() const { return idb->mine_spikes; };
float IDBProjectileAdjust::mine_visibility() const { return idb->mine_visibility; };

float IDBProjectileAdjust::halflife() const { return idb->halflife; };
float IDBProjectileAdjust::airbrake_life() const { CHECK(idb->motion == PM_AIRBRAKE); return idb->airbrake_life; }
float IDBProjectileAdjust::airbrake_slowdown() const { CHECK(idb->motion == PM_AIRBRAKE); return idb->airbrake_slowdown; }

float IDBProjectileAdjust::missile_stabstart() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_stabstart; }
float IDBProjectileAdjust::missile_stabilization() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_stabilization; }
float IDBProjectileAdjust::missile_sidelaunch() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_sidelaunch; }
float IDBProjectileAdjust::missile_backlaunch() const { CHECK(idb->motion == PM_MISSILE); return idb->missile_backlaunch; }

float IDBProjectileAdjust::boomerang_convergence() const { CHECK(idb->motion == PM_BOOMERANG); return idb->boomerang_convergence; }
float IDBProjectileAdjust::boomerang_intersection() const { CHECK(idb->motion == PM_BOOMERANG); return idb->boomerang_intersection; }
float IDBProjectileAdjust::boomerang_maxrotate() const { CHECK(idb->motion == PM_BOOMERANG); return idb->boomerang_maxrotate; }

float IDBProjectileAdjust::dps_duration() const { CHECK(idb->motion == PM_DPS); return idb->dps_duration; }

bool IDBProjectileAdjust::no_intersection() const {
  return idb->no_intersection;
}

float IDBProjectileAdjust::stats_damagePerShot() const {
  float val = 0;
  vector<IDBWarheadAdjust> idbwa = chain_warhead();
  for(int i = 0; i < idbwa.size(); i++)
    val += idbwa[i].stats_damagePerShot();
  vector<IDBDeployAdjust> idbde = chain_deploy();
  for(int i = 0; i < idbde.size(); i++)
    val += idbde[i].stats_damagePerShot();
  return val;
}

void IDBProjectileAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
IDBProjectileAdjust::IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBLauncherAdjust
 */

IDBDeployAdjust IDBLauncherAdjust::deploy() const { return IDBDeployAdjust(idb->deploy, adjust); };

float IDBLauncherAdjust::stats_damagePerShot() const { return deploy().stats_damagePerShot(); };

void IDBLauncherAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
IDBLauncherAdjust::IDBLauncherAdjust(const IDBLauncher *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWeaponAdjust
 */

IDBLauncherAdjust IDBWeaponAdjust::launcher() const { return IDBLauncherAdjust(idb->launcher, adjust); };

pair<int, int> simulateWeaponFiringEngine(float frames_per_shot, Rng *rng) {
  if(frames_per_shot >= 1) {
    return make_pair(1, (int)floor(frames_per_shot) + (rng->frand() < (frames_per_shot - floor(frames_per_shot))));
  } else {
    pair<int, int> swf = simulateWeaponFiringEngine(1 / frames_per_shot, rng);
    return make_pair(swf.second, swf.first);
  }
};

struct FFCETester {
  FFCETester() {
    for(float framn = 0.5; framn <= 1.5; framn += 0.2) {
      const float gole = framn * 1000;
      int tfram = 0;
      Rng rng(unsync().generate_seed());
      for(int i = 0; i < 1000; ) {
        pair<int, int> tfr = simulateWeaponFiringEngine(framn, &rng);
        i += tfr.first;
        tfram += tfr.second;
      }
      dprintf("tfram test: should be %dish, is %d\n", int(gole), tfram);
      
      CHECK(tfram > gole * .95 && tfram < gole * 1.05);
    }
  }
} test;

pair<int, int> IDBWeaponAdjust::simulateWeaponFiring(Rng *rng) const { 
  return simulateWeaponFiringEngine(FPS / firerate(), rng);
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

const IDBWeapon *IDBWeaponAdjust::base() const { return idb; }
void IDBWeaponAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
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

void IDBGloryAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
IDBGloryAdjust::IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBUpgradeAdjust
 */

Money IDBUpgradeAdjust::cost() const { return tank->upgrade_base * idb->costmult / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_UPGRADE); };  // yes, it's based off the tank base cost, not the tank adjusted cost
Money IDBUpgradeAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

void IDBUpgradeAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, tank);
  adler(adl, adjust);
}
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

void IDBBombardmentAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
IDBBombardmentAdjust::IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment &in_adjust, Coord in_bombardlevel) {
  idb = in_idb; adjust = in_adjust;
  if(in_bombardlevel != -1) {
    CHECK(in_bombardlevel >= 0);
    valid_level = true;
    for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
      adjust.adjusts[i] += floor(25 * in_bombardlevel).toInt();
    adjust.adjusts[IDBAdjustment::WARHEAD_RADIUS_FALLOFF] += floor(20 * in_bombardlevel).toInt();
    adjust.adjusts[IDBAdjustment::BOMBARDMENT_SPEED] += floor(25 * in_bombardlevel).toInt();
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

float IDBTankAdjust::maxHealth() const { return idb->health * adjust.tankhp(); };
float IDBTankAdjust::turnSpeed() const { return idb->handling * adjust.adjustmentfactor(IDBAdjustment::TANK_TURN); };
float IDBTankAdjust::maxSpeed() const { return idb->engine * adjust.tankspeed(); };

float IDBTankAdjust::mass() const { return idb->mass; };    // BAM

const vector<Coord2> &IDBTankAdjust::vertices() const { return idb->vertices; };
Coord2 IDBTankAdjust::firepoint() const { return idb->firepoint; };
Coord2 IDBTankAdjust::rearfirepoint() const { return idb->rearfirepoint; };
const vector<Coord2> &IDBTankAdjust::minepath() const { return idb->minepath; };
vector<Coord2> IDBTankAdjust::getTankVertices(Coord2 pos, Coord td) const { return idb->getTankVertices(pos, td); };

const IDBTank *IDBTankAdjust::base() const { return idb; };

void IDBTankAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
IDBTankAdjust::IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs) {
  return lhs.idb == rhs.idb && lhs.adjust == rhs.adjust;
}

/*************
 * IDBImplantSlotAdjust
 */

Money IDBImplantSlotAdjust::cost() const { return idb->cost / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_IMPLANT); };

void IDBImplantSlotAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
}
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

void IDBImplantAdjust::checksum(Adler32 *adl) const {
  adler(adl, idb);
  adler(adl, adjust);
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

float IDBEffectsAdjust::ionblast_radius() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_radius; }
float IDBEffectsAdjust::ionblast_duration() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_duration; }

const vector<pair<int, Color> > &IDBEffectsAdjust::ionblast_visuals() const { CHECK(type() == IDBEffects::EFT_IONBLAST); return idb->ionblast_visuals; }

IDBEffectsAdjust::IDBEffectsAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; }
