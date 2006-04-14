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
  float anglestddev() const;

  IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBWarheadAdjust {
  const IDBWarhead *idb;
  const IDBAdjustment *adjust;
  
public:
  float impactdamage() const;

  float radiusdamage() const;
  float radiusfalloff() const;
  
  float wallremovalradius() const;
  float wallremovalchance() const;

  IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBProjectileAdjust {
  const IDBProjectile *idb;
  const IDBAdjustment *adjust;
  
public:
  int motion() const;
  float velocity() const;

  IDBWarheadAdjust warhead() const;

  Color color() const;
  float width() const;

  IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBWeaponAdjust {
  const IDBWeapon *idb;
  const IDBAdjustment *adjust;

public:
  const string &name() const;

  IDBDeployAdjust deploy() const;
  IDBProjectileAdjust projectile() const;

  int framesForCooldown() const;

  IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBGloryAdjust {
  const IDBGlory *idb;
  const IDBAdjustment *adjust;

public:
  int minsplits() const;
  int maxsplits() const;

  int minsplitsize() const;
  int maxsplitsize() const;

  int shotspersplit() const;
  IDBProjectileAdjust projectile() const;

  IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBUpgradeAdjust {
  const IDBUpgrade *idb;
  const IDBAdjustment *adjust;

public:
  
  IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBBombardmentAdjust {
  const IDBBombardment *idb;
  const IDBAdjustment *adjust;

public:
  int lockdelay() const;
  int unlockdelay() const;

  IDBWarheadAdjust warhead() const;

  IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBTankAdjust {
  const IDBTank *idb;
  const IDBAdjustment *adjust;

public:
  float maxHealth() const;
  float turnSpeed() const;
  float maxSpeed() const;

  IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment *in_adjust);
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
