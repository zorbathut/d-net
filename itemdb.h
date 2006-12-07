#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "color.h"
#include "coord.h"
#include "dvec2.h"
#include "util.h"
#include "rng.h"

#include <map>

using namespace std;

class Player;

/*************
 * Basic data items
 */

#define WARHEAD_RADIUS_MAXMULT 2

const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "warhead_radius_falloff", "discount_weapon", "discount_implant", "discount_upgrade", "discount_tank", "recycle_bonus", "tank_firerate", "tank_speed", "tank_turn", "tank_armor", "damage_all", "all" };
const char * const adjust_human[] = {"Kinetic damage", "Energy damage", "Explosive damage", "Trap damage", "Exotic damage", "Blast radius", "Weapon discount", "Implant discount", "Upgrade discount", "Tank discount", "Recycle bonus", "Tank firerate", "Tank speed", "Tank turning", "Tank armor", "All damage", "All" };
const char * const adjust_unit[] = {" KPE", " KJE", " TOTE", " FSE", " flux", " m", "", "", "", "", "", "", " m/s", " rad/s", " CME"};

string adjust_modifiertext(int id, int amount);

struct IDBAdjustment {
public:
  // NOTE: there is currently an implicit assumption that the "damage" stats are first. Don't violate this. Look for DAMAGE_* for the places where it matters.
  // Also, these are obviously correlated with the string arrays above.
  enum { DAMAGE_KINETIC, DAMAGE_ENERGY, DAMAGE_EXPLOSIVE, DAMAGE_TRAP, DAMAGE_EXOTIC, WARHEAD_RADIUS_FALLOFF, DISCOUNT_WEAPON, DISCOUNT_IMPLANT, DISCOUNT_UPGRADE, DISCOUNT_TANK, RECYCLE_BONUS, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, LAST, DAMAGE_ALL = LAST, ALL, COMBO_LAST, DAMAGE_LAST = DAMAGE_EXOTIC + 1, DISCOUNT_BEGIN = DISCOUNT_WEAPON, DISCOUNT_END = DISCOUNT_TANK + 1, TANK_BEGIN = TANK_FIRERATE, TANK_END = TANK_ARMOR + 1 };
  
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
  Dvec2 icon;
  Color color;
  string name;
  const IDBAdjustment *adjustment[4];

  vector<string> name_lines;

  const string *text;
};

struct IDBEffects {
  int quantity;

  float inertia;
  float spread;

  float slowdown;
  float lifetime;

  float radius;
  Color color;
};

class IDBDeploy; // yay circles
struct IDBWarhead {
  float impactdamage[IDBAdjustment::DAMAGE_LAST];

  float radiusdamage[IDBAdjustment::DAMAGE_LAST];
  float radiusfalloff;
  Color radiuscolor_bright;
  Color radiuscolor_dim;

  float wallremovalradius;
  float wallremovalchance;

  vector<const IDBDeploy *> deploy;

  vector<const IDBEffects *> effects_impact;
};

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE, PM_MINE, PM_LAST };

struct IDBProjectile {
  int motion;
  float velocity;
  float length;
  float radius_physical;

  Color color;
  float thickness_visual;

  float toughness;

  float airbrake_life;
  float halflife;

  vector<const IDBWarhead *> chain_warhead;
};

// Normal specifies "Forward" for tanks, or "Centroid" on cases where there is no tank
enum { DT_NORMAL, DT_FORWARD, DT_CENTROID, DT_MINEPATH, DT_EXPLODE, DT_LAST };

struct IDBDeploy {
  int type;

  float anglestddev;

  int exp_minsplits;
  int exp_maxsplits;
  int exp_minsplitsize;
  int exp_maxsplitsize;

  int exp_shotspersplit;

  vector<const IDBDeploy *> chain_deploy;
  vector<const IDBProjectile *> chain_projectile;
  vector<const IDBWarhead *> chain_warhead;
};

struct IDBStats {
  float dps_efficiency;
  float cps_efficiency;
};

enum { WDM_FIRINGRANGE, WDM_MINES, WDM_LAST };
enum { WFRD_NORMAL, WFRD_MELEE };
struct IDBLauncher {
  const IDBDeploy *deploy;

  int demomode;
  int firingrange_distance;
  
  const IDBStats *stats;
  
  const string *text;
};

struct IDBWeapon {
  string name;

  float firerate;
  const IDBLauncher *launcher;

  Money base_cost;
  int quantity;
};

struct IDBGlory {
  vector<const IDBDeploy *> blast;
  const IDBDeploy *core;

  float demo_range;

  Money base_cost;

  const string *text;
};

struct IDBUpgrade {
  const IDBAdjustment *adjustment;

  float costmult;
  string category;
  
  const IDBUpgrade *prereq;
  bool has_postreq;

  const string *text;
};

struct IDBBombardment {
  const IDBWarhead *warhead;

  float lockdelay;
  float unlockdelay;

  Money base_cost;

  const string *text;
};

struct IDBTank {
  float health;
  float handling;
  float engine;
  float mass;

  const IDBAdjustment *adjustment;
  vector<string> upgrades;

  const IDBWeapon *weapon;

  vector<Coord2> vertices;
  Coord2 firepoint;
  vector<Coord2> minepath;

  Money base_cost;
  Money upgrade_base;

  const string *text;
};

struct IDBShopcache {
  struct Entry {
    int count;
    IDBWarhead *warhead;
    int impact;
    vector<pair<float, int> > distances;
  };
  
  vector<Entry> entries;
  
  int cycles;
  vector<int> tank_specific;
};

struct IDBImplantSlot {
  Money cost;
  
  const IDBImplantSlot *prereq;
  bool has_postreq;
};

struct IDBImplant {
  const IDBAdjustment *adjustment;
};

/*************
 * Adjusted data items
 */

class IDBDeployAdjust;
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

  vector<IDBDeployAdjust> deploy() const;
  const vector<const IDBEffects *> &effects_impact() const;
  
  float stats_damagePerShot() const;

  const IDBWarhead *base() const;

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
  float toughness() const;

  vector<IDBWarheadAdjust> chain_warhead() const;

  Color color() const;
  float thickness_visual() const;

  float halflife() const;

  float airbrake_life() const;
  
  float stats_damagePerShot() const;

  IDBProjectileAdjust(const IDBProjectile *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBDeployAdjust {
  const IDBDeploy *idb;
  IDBAdjustment adjust;
  
public:
  int type() const;

  int exp_minsplits() const;
  int exp_maxsplits() const;

  int exp_minsplitsize() const;
  int exp_maxsplitsize() const;

  int exp_shotspersplit() const;

  float anglestddev() const;
  
  vector<IDBDeployAdjust> chain_deploy() const;
  vector<IDBProjectileAdjust> chain_projectile() const;
  vector<IDBWarheadAdjust> chain_warhead() const;

  float stats_damagePerShot() const;

  IDBDeployAdjust(const IDBDeploy *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBLauncherAdjust {
  const IDBLauncher *idb;
  IDBAdjustment adjust;

public:
  IDBDeployAdjust deploy() const;
  
  float stats_damagePerShot() const;

  IDBLauncherAdjust(const IDBLauncher *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBWeaponAdjust {
  const IDBWeapon *idb;
  IDBAdjustment adjust;

public:
  const string &name() const;

  IDBLauncherAdjust launcher() const;

  int framesForCooldown(Rng *rng) const;
  float firerate() const;
  Money cost(int shots) const;
  Money cost_pack() const;
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
  vector<IDBDeployAdjust> blast() const;
  IDBDeployAdjust core() const;
  
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

  const IDBTank *base() const;

  IDBTankAdjust(const IDBTank *in_idb, const IDBAdjustment &in_adjust);
};
bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs);  // This could exist for others as well, it just doesn't yet.

struct IDBImplantSlotAdjust {
  const IDBImplantSlot *idb;
  IDBAdjustment adjust;
  
public:
  Money cost() const;

  IDBImplantSlotAdjust(const IDBImplantSlot *in_idb, const IDBAdjustment &in_adjust);
};

struct IDBImplantAdjust {
  const IDBImplant *idb;
  IDBAdjustment adjust;
  
public:
  Money costToLevel(int curlevel) const;

  IDBImplantAdjust(const IDBImplant *in_idb, const IDBAdjustment &in_adjust);
};

/*************
 * Hierarchy items
 */

struct HierarchyNode {
public:
  vector<HierarchyNode> branches;

  string name;

  enum Type {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_BOMBARDMENT, HNT_TANK, HNT_IMPLANT, HNT_EQUIP, HNT_EQUIPWEAPON, HNT_SELL, HNT_NONE, HNT_DONE, HNT_LAST};
  Type type;

  enum Displaymode {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_EQUIP, HNDM_IMPLANT_EQUIP, HNDM_IMPLANT_UPGRADE, HNDM_LAST};
  Displaymode displaymode;

  bool buyable;
  
  Money cost(const Player *player) const;
  Money sellvalue(const Player *player) const;
  int pack;
  
  const IDBWeapon *weapon;
  const IDBUpgrade *upgrade;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBTank *tank;
  
  const IDBImplantSlot *implant_slot;
  const IDBImplant *implant_item;
  
  const IDBWeapon *equipweapon;
  
  int cat_restrictiontype;
  Money spawncash;
  
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

const map<string, IDBWeapon> &weaponList();
const map<string, IDBBombardment> &bombardmentList();
const map<string, IDBGlory> &gloryList();
const vector<IDBFaction> &factionList();
const map<string, string> &textList();

bool hasShopcache(const IDBWeapon *weap);
bool hasShopcache(const IDBBombardment *bombard);
bool hasShopcache(const IDBGlory *glory);

const IDBShopcache &getShopcache(const IDBWeapon *weap);
const IDBShopcache &getShopcache(const IDBBombardment *bombard);
const IDBShopcache &getShopcache(const IDBGlory *glory);

const string &nameFromIDB(const IDBWeapon *idbw);
const string &nameFromIDB(const IDBWarhead *idbw);
const string &nameFromIDB(const IDBBombardment *bombard);
const string &nameFromIDB(const IDBGlory *glory);

#endif
