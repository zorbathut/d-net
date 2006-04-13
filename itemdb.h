#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "gfx.h"

#include <string>
#include <vector>

using namespace std;

class Player;

/*************
 * Basic data items
 */

const char * const adjust_text[] = { "damage_proj", "damage_snipe", "damage_explode", "damage_trap", "damage_exotic", "discount_weapon", "discount_training", "discount_upgrade", "discount_license", "discount_tank", "waste_reduction", "tank_firerate", "tank_speed", "tank_turn", "tank_armor" };

struct IDBAdjustment {
public:
  enum { DAMAGE_PROJ, DAMAGE_SNIPE, DAMAGE_EXPLODE, DAMAGE_TRAP, DAMAGE_EXOTIC, DISCOUNT_WEAPON, DISCOUNT_TRAINING, DISCOUNT_UPGRADE, DISCOUNT_LICENSE, DISCOUNT_TANK, WASTE_REDUCTION, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, LAST };
  
  int adjusts[LAST];

  void debugDump();

  IDBAdjustment();
};

const IDBAdjustment &operator+=(IDBAdjustment &lhs, const IDBAdjustment &rhs);

struct IDBFaction {
public:
  Dvec2 icon;
  Color color;
  string name;
  IDBAdjustment *adjustment[4];

  vector<string> name_lines;
};

struct IDBDeploy {
public:
  
  float anglestddev;

  float getDamagePerShotMultiplier() const;
};

struct IDBWarhead {
public:
  float impactdamage;
  float radiusdamage;
  float radiusfalloff;
  float wallremovalradius;
  float wallremovalchance;

  float getDamagePerShot() const;

};

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE };

struct IDBProjectile {
public:
  int motion;
  float velocity;
  Color color;
  float width; // visual effect only
  const IDBWarhead *warhead;

  float getDamagePerShot() const;

};

struct IDBWeapon {
public:
  string name;

  float firerate;
  const IDBDeploy *deploy;
  const IDBProjectile *projectile;

  Money base_cost;
  int quantity;

  float getDamagePerShot() const;
  float getDamagePerSecond() const;
  float getCostPerDamage() const;

  int framesForCooldown() const;
};

struct IDBGlory {
public:
  int minsplits;
  int maxsplits;
  int minsplitsize;
  int maxsplitsize;

  const IDBDeploy *deploy;
  const IDBProjectile *projectile;

  int shotspersplit;

  Money base_cost;

  float getAverageDamage() const;
};

struct IDBUpgrade {
public:
  IDBAdjustment *adjustment;

  Money base_cost;
};

struct IDBBombardment {
public:
  const IDBWarhead *warhead;

  int lockdelay;
  int unlockdelay;

  Money base_cost;
};

struct IDBTank {
};

/*************
 * Adjusted data items
 */

struct IDBDeployAdjust {
  const IDBDeploy *idb;
  const IDBAdjustment *adjust;
  
public:
  float anglestddev() const { return idb->anglestddev; };

  IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBWarheadAdjust {
  const IDBWarhead *idb;
  const IDBAdjustment *adjust;
  
public:
  float impactdamage() const { return idb->impactdamage; };

  float radiusdamage() const { return idb->radiusdamage; };
  float radiusfalloff() const { return idb->radiusfalloff; };
  
  float wallremovalradius() const { return idb->wallremovalradius; };
  float wallremovalchance() const { return idb->wallremovalchance; };

  IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBProjectileAdjust {
  const IDBProjectile *idb;
  const IDBAdjustment *adjust;
  
public:
  int motion() const { return idb->motion; };
  float velocity() const { return idb->velocity; };

  IDBWarheadAdjust warhead() const { return IDBWarheadAdjust(idb->warhead, adjust); };

  Color color() const { return idb->color; };
  float width() const { return idb->width; };

  IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBWeaponAdjust {
  const IDBWeapon *idb;
  const IDBAdjustment *adjust;

public:
  const string &name() const { return idb->name; };

  IDBDeployAdjust deploy() const { return IDBDeployAdjust(idb->deploy, adjust); };
  IDBProjectileAdjust projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };

  int framesForCooldown() const { return idb->framesForCooldown(); };

  IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBGloryAdjust {
  const IDBGlory *idb;
  const IDBAdjustment *adjust;

public:
  int minsplits() const { return idb->minsplits; };
  int maxsplits() const { return idb->maxsplits; };

  int minsplitsize() const { return idb->minsplitsize; };
  int maxsplitsize() const { return idb->maxsplitsize; };

  int shotspersplit() const { return idb->shotspersplit; };
  IDBProjectileAdjust projectile() const { return IDBProjectileAdjust(idb->projectile, adjust); };

  IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBUpgradeAdjust {
  const IDBUpgrade *idb;
  const IDBAdjustment *adjust;

public:
  
  IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBBombardmentAdjust {
  const IDBBombardment *idb;
  const IDBAdjustment *adjust;

public:
  int lockdelay() const { return idb->lockdelay; };
  int unlockdelay() const { return idb->unlockdelay; };

  IDBWarheadAdjust warhead() const { return IDBWarheadAdjust(idb->warhead, adjust); };

  IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

struct IDBTankAdjust {
  const IDBTank *idb;
  const IDBAdjustment *adjust;

public:
  float maxHealth() const { return 20; };
  float turnSpeed() const { return 2.f / FPS; };
  float maxSpeed() const { return 24.f / FPS; };

  IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment *in_adjust) { idb = in_idb; adjust = in_adjust; };
};

/*************
 * Hierarchy items
 */

struct HierarchyNode {
public:
  vector<HierarchyNode> branches;

  string name;

  enum {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_BOMBARDMENT, HNT_DONE, HNT_LAST};
  int type;

  enum {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_LAST};
  int displaymode;

  bool buyable;
  
  Money cost(const Player *player) const;
  int pack;
  
  const IDBWeapon *weapon;
  const IDBUpgrade *upgrade;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  
  int cat_restrictiontype;
  
  void checkConsistency() const;
  
  HierarchyNode();
  
};

void initItemdb();

/*************
 * Accessors
 */

const HierarchyNode &itemDbRoot();

const IDBWeapon *defaultWeapon();
const IDBGlory *defaultGlory();
const IDBBombardment *defaultBombardment();
const vector<IDBFaction> &factionList();

#endif
