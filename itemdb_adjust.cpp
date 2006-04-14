
#include "itemdb.h"

#include "rng.h"

/*************
 * IDBDeployAdjust
 */
 
float IDBDeployAdjust::anglestddev() const { return idb->anglestddev; };

IDBDeployAdjust::IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWarheadAdjust
 */

float IDBWarheadAdjust::impactdamage() const { return idb->impactdamage * adjust->adjustmentfactor(idb->type); };

float IDBWarheadAdjust::radiusdamage() const { return idb->radiusdamage * adjust->adjustmentfactor(idb->type); };
float IDBWarheadAdjust::radiusfalloff() const { return idb->radiusfalloff; };

float IDBWarheadAdjust::wallremovalradius() const { return idb->wallremovalradius; };
float IDBWarheadAdjust::wallremovalchance() const { return idb->wallremovalchance; };

IDBWarheadAdjust::IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBProjectileAdjust
 */

int IDBProjectileAdjust::motion() const { return idb->motion; };
float IDBProjectileAdjust::velocity() const { return idb->velocity; };

IDBWarheadAdjust IDBProjectileAdjust::warhead() const { return IDBWarheadAdjust(idb->warhead, adjust); };

Color IDBProjectileAdjust::color() const { return idb->color; };
float IDBProjectileAdjust::width() const { return idb->width; };

IDBProjectileAdjust::IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBWeaponAdjust
 */

const string &IDBWeaponAdjust::name() const { return idb->name; };

IDBDeployAdjust IDBWeaponAdjust::deploy() const { return IDBDeployAdjust(idb->deploy, adjust); };
IDBProjectileAdjust IDBWeaponAdjust::projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };

int IDBWeaponAdjust::framesForCooldown() const { 
  float frames_per_shot = FPS / (idb->firerate * adjust->adjustmentfactor(IDBAdjustment::TANK_FIRERATE));
  return (int)floor(frames_per_shot) + (frand() < (frames_per_shot - floor(frames_per_shot)));
}

IDBWeaponAdjust::IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBGloryAdjust
 */

int IDBGloryAdjust::minsplits() const { return idb->minsplits; };
int IDBGloryAdjust::maxsplits() const { return idb->maxsplits; };

int IDBGloryAdjust::minsplitsize() const { return idb->minsplitsize; };
int IDBGloryAdjust::maxsplitsize() const { return idb->maxsplitsize; };

int IDBGloryAdjust::shotspersplit() const { return idb->shotspersplit; };
IDBProjectileAdjust IDBGloryAdjust::projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };

IDBGloryAdjust::IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBUpgradeAdjust
 */

IDBUpgradeAdjust::IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBBombardmentAdjust
 */

int IDBBombardmentAdjust::lockdelay() const { return idb->lockdelay; };
int IDBBombardmentAdjust::unlockdelay() const { return idb->unlockdelay; };

IDBWarheadAdjust IDBBombardmentAdjust::warhead() const { return IDBWarheadAdjust(idb->warhead, adjust); };

IDBBombardmentAdjust::IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };

/*************
 * IDBTankAdjust
 */

float IDBTankAdjust::maxHealth() const { return 20 * adjust->adjustmentfactor(IDBAdjustment::TANK_ARMOR); };
float IDBTankAdjust::turnSpeed() const { return 2.f / FPS * adjust->adjustmentfactor(IDBAdjustment::TANK_TURN); };
float IDBTankAdjust::maxSpeed() const { return 24.f / FPS * adjust->adjustmentfactor(IDBAdjustment::TANK_SPEED); };

IDBTankAdjust::IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
