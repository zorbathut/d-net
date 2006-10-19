#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "color.h"
#include "coord.h"
#include "dvec2.h"
#include "util.h"

using namespace std;

class Player;

/*************
 * Basic data items
 */

const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "warhead_radius_falloff", "discount_weapon", "discount_training", "discount_upgrade", "discount_tank", "recycle_bonus", "tank_firerate", "tank_speed", "tank_turn", "tank_armor", "damage_all", "all" };
const char * const adjust_human[] = {"Kinetic damage", "Energy damage", "Explosive damage", "Trap damage", "Exotic damage", "Blast radius", "Weapon discount", "Training discount", "Upgrade discount", "Tank discount", "Recycle bonus", "Tank firerate", "Tank speed", "Tank turning", "Tank armor", "All damage", "All" };
const char * const adjust_unit[] = {" KPE", " KJE", " TOTE", " FSE", " flux", " m", "", "", "", "", "", "", " m/s", " rad/s", " CME"};

string adjust_modifiertext(int id, int amount);

struct IDBAdjustment {
public:
  // NOTE: there is currently an implicit assumption that the "damage" stats are first. Don't violate this. Look for DAMAGE_* for the places where it matters.
  enum { DAMAGE_KINETIC, DAMAGE_ENERGY, DAMAGE_EXPLOSIVE, DAMAGE_TRAP, DAMAGE_EXOTIC, WARHEAD_RADIUS_FALLOFF, DISCOUNT_WEAPON, DISCOUNT_TRAINING, DISCOUNT_UPGRADE, DISCOUNT_TANK, RECYCLE_BONUS, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, LAST, DAMAGE_ALL = LAST, ALL, COMBO_LAST, DAMAGE_LAST = DAMAGE_EXOTIC + 1, DISCOUNT_BEGIN = DISCOUNT_WEAPON, DISCOUNT_END = DISCOUNT_TANK + 1, TANK_BEGIN = TANK_FIRERATE, TANK_END = TANK_ARMOR + 1 };
  
  int adjusts[LAST];  // These are in percentage points away from 100. Yes, this is kind of weird.
  
  pair<int, int> adjustlist[5];
  
  float adjustmentfactor(int type) const;
  float recyclevalue() const; // this is annoyingly different

  void debugDump();

  IDBAdjustment();
};

const IDBAdjustment &operator+=(IDBAdjustment &lhs, const IDBAdjustment &rhs);
bool operator==(const IDBAdjustment &lhs, const IDBAdjustment &rhs);

struct IDBFaction {
public:
  Dvec2 icon;
  Color color;
  string name;
  const IDBAdjustment *adjustment[4];

  vector<string> name_lines;

  const string *text;
};

// Normal specifies "Forward" for tanks, or "Centroid" on cases where there is no tank
enum { DT_NORMAL, DT_FORWARD, DT_CENTROID, DT_MINEPATH, DT_LAST };

struct IDBDeploy {
public:
  int type;

  float anglestddev;
};

struct IDBWarhead {
public:
  float impactdamage[IDBAdjustment::DAMAGE_LAST];

  float radiusdamage[IDBAdjustment::DAMAGE_LAST];
  float radiusfalloff;
  Color radiuscolor_bright;
  Color radiuscolor_dim;

  float wallremovalradius;
  float wallremovalchance;
};

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE, PM_MINE, PM_INSTANT, PM_LAST };

struct IDBProjectile {
public:
  int motion;
  float velocity;
  float length;
  float radius_physical;

  Color color;
  float thickness_visual;

  float halflife;

  const IDBWarhead *warhead;
};

struct IDBStats {
public:
  float dps_efficiency;
  float cps_efficiency;
};

enum { WDM_FIRINGRANGE, WDM_MINES, WDM_LAST };
enum { WFRD_NORMAL, WFRD_MELEE };
struct IDBLauncher {
  const IDBDeploy *deploy;
  const IDBProjectile *projectile;

  int demomode;
  int firingrange_distance;
  
  const IDBStats *stats;
  
  const string *text;
};

struct IDBWeapon {
public:
  string name;

  float firerate;
  const IDBLauncher *launcher;

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
  const IDBLauncher *launcher;

  int shotspersplit;

  Money base_cost;

  const string *text;
};

struct IDBUpgrade {
public:
  const IDBAdjustment *adjustment;

  int costmult;

  const string *text;
};

struct IDBBombardment {
public:
  const IDBWarhead *warhead;

  float lockdelay;
  float unlockdelay;

  Money base_cost;

  const string *text;
};

struct IDBTank {
public:
  float health;
  float handling;
  float engine;
  float mass;

  const IDBWeapon *weapon;

  vector<Coord2> vertices;
  Coord2 firepoint;
  vector<Coord2> minepath;

  Money base_cost;
  Money upgrade_base;

  const string *text;
};

/*************
 * Adjusted data items
 */

struct IDBDeployAdjust {
  const IDBDeploy *idb;
  IDBAdjustment adjust;
  
public:
  int type() const;

  float anglestddev() const;
  
  float stats_damagePerShotMultiplier() const;

  IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBWarheadAdjust {
  const IDBWarhead *idb;
  IDBAdjustment adjust;
  
  float accumulate(const float *damage) const;
  
public:
  float impactdamage() const;

  float radiusdamage() const;
  float radiusfalloff() const;
  Color radiuscolor_bright() const;
  Color radiuscolor_dim() const;
  
  float wallremovalradius() const;
  float wallremovalchance() const;
  
  float stats_damagePerShot() const;

  IDBWarheadAdjust(const IDBWarhead *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBProjectileAdjust {
  const IDBProjectile *idb;
  IDBAdjustment adjust;
  
public:
  int motion() const;
  float velocity() const;
  float length() const;
  float radius_physical() const;

  IDBWarheadAdjust warhead() const;

  Color color() const;
  float thickness_visual() const;

  float halflife() const;
  
  float stats_damagePerShot() const;

  IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBLauncherAdjust {
  const IDBLauncher *idb;
  IDBAdjustment adjust;

public:
  IDBDeployAdjust deploy() const;
  IDBProjectileAdjust projectile() const;
  
  float stats_damagePerShot() const;

  IDBLauncherAdjust(const IDBLauncher *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBWeaponAdjust {
  const IDBWeapon *idb;
  IDBAdjustment adjust;

public:
  const string &name() const;

  IDBLauncherAdjust launcher() const;

  int framesForCooldown() const;
  float firerate() const;
  Money cost() const;
  Money sellcost(int shots) const;

  float stats_damagePerSecond() const;
  float stats_costPerDamage() const;
  float stats_costPerSecond() const;

  IDBWeaponAdjust(const IDBWeapon *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBGloryAdjust {
  const IDBGlory *idb;
  IDBAdjustment adjust;

public:
  int minsplits() const;
  int maxsplits() const;

  int minsplitsize() const;
  int maxsplitsize() const;

  int shotspersplit() const;
  IDBProjectileAdjust projectile() const;
  IDBLauncherAdjust launcher() const;
  
  Money cost() const;
  Money sellcost() const;

  float stats_averageDamage() const;
  
  IDBGloryAdjust(const IDBGlory *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBUpgradeAdjust {
  const IDBUpgrade *idb;
  IDBAdjustment adjust;
  const IDBTank *tank;

public:

  Money cost() const;
  Money sellcost() const;

  IDBUpgradeAdjust(const IDBUpgrade *in_idb, const IDBTank *in_tank, const IDBAdjustment &in_adjust);
};

struct IDBBombardmentAdjust {
  const IDBBombardment *idb;
  IDBAdjustment adjust;
  bool valid_level;

public:
  float lockdelay() const;
  float unlockdelay() const;

  IDBWarheadAdjust warhead() const;
  
  Money cost() const;
  Money sellcost() const;

  IDBBombardmentAdjust(const IDBBombardment *in_idb, const IDBAdjustment &in_adjust, int blevel);
};

struct IDBTankAdjust {
  const IDBTank *idb;
  IDBAdjustment adjust;

  friend bool operator==(const IDBTankAdjust &, const IDBTankAdjust &);
public:
  float maxHealth() const;
  float turnSpeed() const;
  float maxSpeed() const;

  float mass() const;

  const vector<Coord2> &vertices() const;
  Coord2 firepoint() const;
  const vector<Coord2> &minepath() const;

  Money cost() const;
  Money sellcost() const;

  IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment &in_adjust);
};
bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs);  // This could exist for others as well, it just doesn't yet.

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
  Money spawncash;
  
  void checkConsistency() const;
  
  HierarchyNode();
  
};

void initItemdb();
void generateCachedShops();
void generateWeaponStats();
void generateFactionStats();

/*************
 * Accessors
 */

const HierarchyNode &itemDbRoot();

const IDBTank *defaultTank();
const IDBGlory *defaultGlory();
const IDBBombardment *defaultBombardment();
const vector<IDBFaction> &factionList();

#endif
