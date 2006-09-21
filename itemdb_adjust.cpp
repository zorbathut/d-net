
#include "itemdb.h"

#include "const.h"
#include "rng.h"

using namespace std;

/*************
 * IDBDeployAdjust
 */
 
int IDBDeployAdjust::type() const { return idb->type; };

float IDBDeployAdjust::anglestddev() const { return idb->anglestddev; };

float IDBDeployAdjust::stats_damagePerShotMultiplier() const { return 1; }; // IN ALL POSSIBLE CASES

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

float IDBWarheadAdjust::impactdamage() const { return accumulate(idb->impactdamage); };

float IDBWarheadAdjust::radiusdamage() const { return accumulate(idb->radiusdamage); };
float IDBWarheadAdjust::radiusfalloff() const { return adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->radiusfalloff; };

float IDBWarheadAdjust::wallremovalradius() const { return adjust.adjustmentfactor(IDBAdjustment::WARHEAD_RADIUS_FALLOFF) * idb->wallremovalradius; };  // just 'cause :)
float IDBWarheadAdjust::wallremovalchance() const { return idb->wallremovalchance; };
Color IDBWarheadAdjust::radiuscolor_bright() const { return idb->radiuscolor_bright; };
Color IDBWarheadAdjust::radiuscolor_dim() const { return idb->radiuscolor_dim; };

float IDBWarheadAdjust::stats_damagePerShot() const { return impactdamage() + radiusdamage(); };

IDBWarheadAdjust::IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBProjectileAdjust
 */

int IDBProjectileAdjust::motion() const { return idb->motion; };
float IDBProjectileAdjust::velocity() const { return idb->velocity; };
float IDBProjectileAdjust::radius_physical() const { return idb->radius_physical; };

IDBWarheadAdjust IDBProjectileAdjust::warhead() const { return IDBWarheadAdjust(idb->warhead, adjust); };

Color IDBProjectileAdjust::color() const { return idb->color; };
float IDBProjectileAdjust::thickness_visual() const { return idb->thickness_visual; };

float IDBProjectileAdjust::stats_damagePerShot() const { return warhead().stats_damagePerShot(); };

IDBProjectileAdjust::IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWeaponAdjust
 */

const string &IDBWeaponAdjust::name() const { return idb->name; };

IDBDeployAdjust IDBWeaponAdjust::deploy() const { return IDBDeployAdjust(idb->deploy, adjust); };
IDBProjectileAdjust IDBWeaponAdjust::projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };

int framesForCooldownEngine(float frames_per_shot) {
  return (int)floor(frames_per_shot) + (frand() < (frames_per_shot - floor(frames_per_shot)));
};

struct FFCETester {
  FFCETester() {
    const float framn = 2;
    const float gole = framn * 1000;
    int tfram = 0;
    for(int i = 0; i < 1000; i++)
      tfram += framesForCooldownEngine(framn);
    dprintf("tfram test: should be %dish, is %d\n", int(gole), tfram);
    
    CHECK(tfram > gole * .9 && tfram < gole * 1.1);
  }
} test;

int IDBWeaponAdjust::framesForCooldown() const { 
  return framesForCooldownEngine(FPS / firerate());
}
float IDBWeaponAdjust::firerate() const {
  return idb->firerate * adjust.adjustmentfactor(IDBAdjustment::TANK_FIRERATE);
}
Money IDBWeaponAdjust::cost() const { return idb->base_cost / adjust.adjustmentfactor(IDBAdjustment::DISCOUNT_WEAPON); };
Money IDBWeaponAdjust::sellcost(int amount) const { return cost() * adjust.recyclevalue() * amount / idb->quantity; };

float IDBWeaponAdjust::stats_damagePerShot() const { return deploy().stats_damagePerShotMultiplier() * projectile().stats_damagePerShot(); }
float IDBWeaponAdjust::stats_damagePerSecond() const { return stats_damagePerShot() * firerate(); }
float IDBWeaponAdjust::stats_costPerDamage() const { return cost().toFloat() / idb->quantity / stats_damagePerShot(); }
float IDBWeaponAdjust::stats_costPerSecond() const { return cost().toFloat() / idb->quantity * firerate(); }

IDBWeaponAdjust::IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBGloryAdjust
 */

int IDBGloryAdjust::minsplits() const { return idb->minsplits; };
int IDBGloryAdjust::maxsplits() const { return idb->maxsplits; };

int IDBGloryAdjust::minsplitsize() const { return idb->minsplitsize; };
int IDBGloryAdjust::maxsplitsize() const { return idb->maxsplitsize; };

int IDBGloryAdjust::shotspersplit() const { return idb->shotspersplit; };
IDBProjectileAdjust IDBGloryAdjust::projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };
IDBWarheadAdjust IDBGloryAdjust::core() const { return IDBWarheadAdjust(idb->core, adjust); };

Money IDBGloryAdjust::cost() const { return idb->base_cost; };
Money IDBGloryAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

float IDBGloryAdjust::stats_averageDamage() const { return (minsplits() + maxsplits()) / 2.0 * shotspersplit() * projectile().stats_damagePerShot() + core().stats_damagePerShot(); }

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

float IDBBombardmentAdjust::lockdelay() const { return idb->lockdelay; };
float IDBBombardmentAdjust::unlockdelay() const { return idb->unlockdelay; };

IDBWarheadAdjust IDBBombardmentAdjust::warhead() const { CHECK(valid_level); return IDBWarheadAdjust(idb->warhead, adjust); };

Money IDBBombardmentAdjust::cost() const { return idb->base_cost; };
Money IDBBombardmentAdjust::sellcost() const { return cost() * adjust.recyclevalue(); };

IDBBombardmentAdjust::IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment &in_adjust, int in_bombardlevel) {
  idb = in_idb; adjust = in_adjust;
  if(in_bombardlevel != -1) {
    CHECK(in_bombardlevel >= 0);
    valid_level = true;
    for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
      adjust.adjusts[i] += 25 * in_bombardlevel;
    adjust.adjusts[IDBAdjustment::WARHEAD_RADIUS_FALLOFF] += 20 * in_bombardlevel;
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

IDBTankAdjust::IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment &in_adjust) { idb = in_idb; adjust = in_adjust; };

bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs) {
  return lhs.idb == rhs.idb && lhs.adjust == rhs.adjust;
}
