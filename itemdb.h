#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "dvec2.h"
#include "coord.h"
#include "util.h"

using namespace std;

class Player;

/*************
 * Basic data items
 */

const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "discount_weapon", "discount_training", "discount_upgrade", "discount_license", "discount_tank", "recycle_bonus", "tank_firerate", "tank_speed", "tank_turn", "tank_armor" };
const char * const adjust_human[] = {"Kinetic damage", "Energy damage", "Explosive damage", "Trap damage", "Exotic damage", "Weapon discount", "Training discount", "Upgrade discount", "License discount", "Tank discount", "Recycle bonus", "Tank firerate", "Tank speed", "Tank turning", "Tank armor" };

struct IDBAdjustment {
public:
  // NOTE: there is currently an implicit assumption that the "damage" stats are first. Don't violate this. Look for DAMAGE_* for the places where it matters.
  enum { DAMAGE_KINETIC, DAMAGE_ENERGY, DAMAGE_EXPLOSIVE, DAMAGE_TRAP, DAMAGE_EXOTIC, DISCOUNT_WEAPON, DISCOUNT_TRAINING, DISCOUNT_UPGRADE, DISCOUNT_LICENSE, DISCOUNT_TANK, RECYCLE_BONUS, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, LAST, DAMAGE_LAST = DAMAGE_EXOTIC + 1 };
  
  int adjusts[LAST];
  
  float adjustmentfactor(int type) const;
  float recyclevalue() const; // this is annoyingly different

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
};

struct IDBWarhead {
public:
  float impactdamage[IDBAdjustment::DAMAGE_LAST];
  float radiusdamage[IDBAdjustment::DAMAGE_LAST];
  float radiusfalloff;
  float wallremovalradius;
  float wallremovalchance;
};

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE };

struct IDBProjectile {
public:
  int motion;
  float velocity;
  Color color;
  float width; // visual effect only
  const IDBWarhead *warhead;
};

struct IDBWeapon {
public:
  string name;

  float firerate;
  const IDBDeploy *deploy;
  const IDBProjectile *projectile;

  Money base_cost;
  int quantity;
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
};

struct IDBUpgrade {
public:
  IDBAdjustment *adjustment;

  int costmult;
};

struct IDBBombardment {
public:
  const IDBWarhead *warhead;

  float lockdelay;
  float unlockdelay;

  Money base_cost;
};

struct IDBTank {
public:
  float health;
  float handling;
  float engine;

  const IDBWeapon *weapon;
  vector<Coord2> vertices;

  Money base_cost;
  Money upgrade_base;
};

/*************
 * Adjusted data items
 */

struct IDBDeployAdjust {
  const IDBDeploy *idb;
  const IDBAdjustment *adjust;
  
public:
  float anglestddev() const;
  
  float stats_damagePerShotMultiplier() const;

  IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBWarheadAdjust {
  const IDBWarhead *idb;
  const IDBAdjustment *adjust;
  
  float accumulate(const float *damage) const;
  
public:
  float impactdamage() const;

  float radiusdamage() const;
  float radiusfalloff() const;
  
  float wallremovalradius() const;
  float wallremovalchance() const;
  
  float stats_damagePerShot() const;

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
  
  float stats_damagePerShot() const;

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
  float firerate() const;
  Money cost() const;
  Money sellcost(int shots) const;
  
  float stats_damagePerShot() const;
  float stats_damagePerSecond() const;
  float stats_costPerDamage() const;
  float stats_costPerSecond() const;

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
  
  Money cost() const;
  Money sellcost() const;

  float stats_averageDamage() const;
  
  IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBUpgradeAdjust {
  const IDBUpgrade *idb;
  const IDBAdjustment *adjust;
  const IDBTank *tank;

public:

  Money cost() const;
  Money sellcost() const;

  IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBTank *in_tank, const IDBAdjustment *in_adjust);
};

struct IDBBombardmentAdjust {
  const IDBBombardment *idb;
  const IDBAdjustment *adjust;

public:
  float lockdelay() const;
  float unlockdelay() const;

  IDBWarheadAdjust warhead() const;
  
  Money cost() const;
  Money sellcost() const;

  IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment *in_adjust);
};

struct IDBTankAdjust {
  const IDBTank *idb;
  const IDBAdjustment *adjust;

public:
  float maxHealth() const;
  float turnSpeed() const;
  float maxSpeed() const;

  const vector<Coord2> &vertices() const;

  Money cost() const;
  Money sellcost() const;

  IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment *in_adjust);
};

/*************
 * Hierarchy items
 */

struct HierarchyNode {
public:
  vector<HierarchyNode> branches;

  string name;

  enum {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_BOMBARDMENT, HNT_TANK, HNT_EQUIP, HNT_EQUIPWEAPON, HNT_DONE, HNT_LAST};
  int type;

  enum {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_EQUIP, HNDM_LAST};
  int displaymode;

  bool buyable;
  
  Money cost(const Player *player) const;
  Money sellvalue(const Player *player) const;
  int pack;
  
  const IDBWeapon *weapon;
  const IDBUpgrade *upgrade;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBTank *tank;
  
  const IDBWeapon *equipweapon;
  
  int cat_restrictiontype;
  
  void checkConsistency() const;
  
  HierarchyNode();
  
};

void initItemdb();

/*************
 * Accessors
 */

const HierarchyNode &itemDbRoot();

const IDBTank *defaultTank();
const IDBGlory *defaultGlory();
const IDBBombardment *defaultBombardment();
const vector<IDBFaction> &factionList();

#endif
