#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "color.h"
#include "coord.h"
#include "dvec2.h"
#include "util.h"
#include "rng.h"

#include <map>

using namespace std;

/*************
 * Basic data items
 */

#define WARHEAD_RADIUS_MAXMULT 2

const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "warhead_radius_falloff", "discount_weapon", "discount_implant", "discount_upgrade", "discount_tank", "recycle_bonus", "tank_firerate", "tank_speed", "tank_turn", "tank_armor", "bombardment_speed", "damage_all", "all" };
const char * const adjust_human[] = {"Kinetic damage", "Energy damage", "Explosive damage", "Trap damage", "Exotic damage", "Blast radius", "Weapon discount", "Implant discount", "Upgrade discount", "Tank discount", "Recycle bonus", "Tank firerate", "Tank speed", "Tank turning", "Tank armor", "Bombardment speed", "All damage", "All" };
const char * const adjust_unit[] = {" KPE", " KJE", " TOTE", " FSE", " flux", " m", "", "", "", "", "", "", " m/s", " rad/s", " CME", ""};

string adjust_modifiertext(int id, int amount);

struct IDBAdjustment {
public:
  // NOTE: there is currently an implicit assumption that the "damage" stats are first. Don't violate this. Look for DAMAGE_* for the places where it matters.
  // Also, these are obviously correlated with the string arrays above.
  enum { DAMAGE_KINETIC, DAMAGE_ENERGY, DAMAGE_EXPLOSIVE, DAMAGE_TRAP, DAMAGE_EXOTIC, WARHEAD_RADIUS_FALLOFF, DISCOUNT_WEAPON, DISCOUNT_IMPLANT, DISCOUNT_UPGRADE, DISCOUNT_TANK, RECYCLE_BONUS, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, BOMBARDMENT_SPEED, LAST, DAMAGE_ALL = LAST, ALL, COMBO_LAST, DAMAGE_LAST = DAMAGE_EXOTIC + 1, DISCOUNT_BEGIN = DISCOUNT_WEAPON, DISCOUNT_END = DISCOUNT_TANK + 1, TANK_BEGIN = TANK_FIRERATE, TANK_END = TANK_ARMOR + 1 };
  
  bool ignore_excessive_radius;
  int adjusts[LAST];  // These are in percentage points away from 100. Yes, this is kind of weird.
  
  pair<int, int> adjustlist[5];
  
  float adjustmentfactor(int type) const;
  float recyclevalue() const; // this is annoyingly different

  void debugDump() const;

  IDBAdjustment();
};

IDBAdjustment operator*(const IDBAdjustment &lhs, int mult);
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
  enum IDBEType { EFT_PARTICLE, EFT_IONBLAST };
  
  IDBEType type;
  int quantity;

  // percentage
  float particle_inertia;
  float particle_reflect;
  
  // m/s (times gaussian)
  float particle_spread;

  float particle_slowdown;
  float particle_lifetime;

  float particle_radius;
  Color particle_color;
  
  float ionblast_radius;
  float ionblast_duration;
  
  vector<pair<int, Color> > ionblast_visuals;
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

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE, PM_MINE, PM_DPS, PM_LAST };

struct IDBProjectile {
  int motion;
  float velocity;
  float length;
  float radius_physical;

  Color color;
  float thickness_visual;
  int mine_spikes;

  float toughness;

  float halflife;
  
  float missile_sidelaunch;
  float missile_backlaunch;
  float missile_stabstart;
  float missile_stabilization;
  
  float airbrake_life;
  
  float dps_duration;

  vector<const IDBWarhead *> chain_warhead;
};

// Normal specifies "Forward" for tanks, or "Centroid" on cases where there is no tank
enum { DT_NORMAL, DT_FORWARD, DT_REAR, DT_CENTROID, DT_MINEPATH, DT_EXPLODE, DT_LAST };

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

enum { WDM_FIRINGRANGE, WDM_MINES, WDM_LAST };
enum { WFRD_NORMAL, WFRD_MELEE };
struct IDBLauncher {
  const IDBDeploy *deploy;

  int demomode;
  int firingrange_distance;
  
  const string *text;
};

struct IDBWeapon {
  string name;

  float firerate;
  const IDBLauncher *launcher;

  Money base_cost;
  int quantity;
  
  bool glory_resistance;
  bool nocache;
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
  vector<const IDBWarhead *> warheads;
  vector<const IDBProjectile *> projectiles;
  vector<const IDBEffects *> effects;
  
  bool showdirection;

  float lockdelay;
  float unlockdelay;

  Money cost;

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
  Coord2 rearfirepoint;
  vector<Coord2> minepath;

  Money base_cost;
  Money upgrade_base;

  const string *text;
  
  vector<Coord2> getTankVertices(Coord2 pos, float td) const;
  
  Coord2 centering_adjustment;
};

struct IDBShopcache {
  struct Entry {
    int count;
    IDBWarhead *warhead;
    float mult;
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
  
  const string *text;
};

struct IDBImplant {
  const IDBAdjustment *adjustment;
  
  const string *text;
  
  IDBAdjustment makeAdjustment(int level) const;
};

/*************
 * Adjusted data items
 */

template<typename T> class IDBItemProperties;

class IDBDeployAdjust;
class IDBEffectsAdjust;
struct IDBWarheadAdjust {
  const IDBWarhead *idb;
  IDBAdjustment adjust;
  float mf;
  
  float accumulate(const float *damage) const;
  
public:
  typedef IDBWarhead base_type;

  float impactdamage() const;

  float radiusdamage() const;
  float radiusfalloff() const;
  Color radiuscolor_bright() const;
  Color radiuscolor_dim() const;
  
  float wallremovalradius() const;
  float wallremovalchance() const;

  vector<IDBDeployAdjust> deploy() const;
  vector<IDBEffectsAdjust> effects_impact() const;
  
  float stats_damagePerShot() const;
  float stats_damagePerShotType(int type) const;

  const IDBWarhead *base() const;
  float multfactor() const;
  IDBWarheadAdjust multiply(float mult) const;

  IDBWarheadAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust, float in_multfactor = 1.0f);
};
template<> struct IDBItemProperties<IDBWarheadAdjust::base_type> {
  typedef IDBWarheadAdjust adjusted;
};

struct IDBProjectileAdjust {
  const IDBProjectile *idb;
  IDBAdjustment adjust;
  
public:
  typedef IDBProjectile base_type;

  int motion() const;
  float velocity() const;
  float length() const;
  float radius_physical() const;
  float toughness() const;

  vector<IDBWarheadAdjust> chain_warhead(float multfactor = 1.0f) const;

  Color color() const;
  float thickness_visual() const;
  int mine_spikes() const;

  float halflife() const;

  float airbrake_life() const;

  float missile_stabstart() const;
  float missile_stabilization() const;
  float missile_sidelaunch() const;
  float missile_backlaunch() const;

  float dps_duration() const;
  
  float stats_damagePerShot() const;

  IDBProjectileAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBProjectileAdjust::base_type> {
  typedef IDBProjectileAdjust adjusted;
};

struct IDBDeployAdjust {
  const IDBDeploy *idb;
  IDBAdjustment adjust;
  
public:
  typedef IDBDeploy base_type;

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

  IDBDeployAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBDeployAdjust::base_type> {
  typedef IDBDeployAdjust adjusted;
};

struct IDBLauncherAdjust {
  const IDBLauncher *idb;
  IDBAdjustment adjust;

public:
  typedef IDBLauncher base_type;

  IDBDeployAdjust deploy() const;
  
  float stats_damagePerShot() const;

  IDBLauncherAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBLauncherAdjust::base_type> {
  typedef IDBLauncherAdjust adjusted;
};

struct IDBWeaponAdjust {
  const IDBWeapon *idb;
  IDBAdjustment adjust;

public:
  typedef IDBWeapon base_type;

  const string &name() const;

  IDBLauncherAdjust launcher() const;

  int framesForCooldown(Rng *rng) const;
  float firerate() const;
  Money cost(int shots) const;
  Money cost_pack() const;
  Money sellcost(int shots) const;

  bool glory_resistance() const;

  float stats_damagePerSecond() const;
  float stats_damagePerSecondType(int type) const;

  float stats_costPerDamage() const;
  float stats_costPerSecond() const;

  IDBWeaponAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBWeaponAdjust::base_type> {
  typedef IDBWeaponAdjust adjusted;
};

struct IDBGloryAdjust {
  const IDBGlory *idb;
  IDBAdjustment adjust;

public:
  typedef IDBGlory base_type;

  vector<IDBDeployAdjust> blast() const;
  IDBDeployAdjust core() const;
  
  Money cost() const;
  Money sellcost() const;

  float stats_averageDamage() const;
  
  IDBGloryAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBGloryAdjust::base_type> {
  typedef IDBGloryAdjust adjusted;
};

struct IDBUpgradeAdjust {
  const IDBUpgrade *idb;
  IDBAdjustment adjust;
  const IDBTank *tank;

public:
  typedef IDBUpgrade base_type;

  Money cost() const;
  Money sellcost() const;

  IDBUpgradeAdjust(const base_type *in_idb, const IDBTank *in_tank, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBUpgradeAdjust::base_type> {
  typedef IDBUpgradeAdjust adjusted;
};

struct IDBBombardmentAdjust {
  const IDBBombardment *idb;
  IDBAdjustment adjust;
  bool valid_level;

public:
  typedef IDBBombardment base_type;

  float lockdelay() const;
  float unlockdelay() const;

  bool showdirection() const;

  vector<IDBWarheadAdjust> warheads() const;
  vector<IDBProjectileAdjust> projectiles() const;
  vector<IDBEffectsAdjust> effects() const;
  
  Money cost() const;
  Money sellcost() const;

  float stats_damagePerShot() const;
  float stats_damagePerShotType(int type) const;

  IDBBombardmentAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust, int blevel);
};
template<> struct IDBItemProperties<IDBBombardmentAdjust::base_type> {
  typedef IDBBombardmentAdjust adjusted;
};

struct IDBTankAdjust {
  const IDBTank *idb;
  IDBAdjustment adjust;

  friend bool operator==(const IDBTankAdjust &, const IDBTankAdjust &);
public:
  typedef IDBTank base_type;

  float maxHealth() const;
  float turnSpeed() const;
  float maxSpeed() const;

  float mass() const;

  const vector<Coord2> &vertices() const;
  Coord2 firepoint() const;
  Coord2 rearfirepoint() const;
  const vector<Coord2> &minepath() const;
  vector<Coord2> getTankVertices(Coord2 pos, float td) const;

  Money cost() const;
  Money sellcost() const;

  const IDBTank *base() const;

  IDBTankAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBTankAdjust::base_type> {
  typedef IDBTankAdjust adjusted;
};
bool operator==(const IDBTankAdjust &lhs, const IDBTankAdjust &rhs);  // This could exist for others as well, it just doesn't yet.

struct IDBImplantSlotAdjust {
  const IDBImplantSlot *idb;
  IDBAdjustment adjust;
  
public:
  typedef IDBImplantSlot base_type;

  Money cost() const;

  IDBImplantSlotAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBImplantSlotAdjust::base_type> {
  typedef IDBImplantSlotAdjust adjusted;
};

struct IDBImplantAdjust {
  const IDBImplant *idb;
  IDBAdjustment adjust;
  
public:
  typedef IDBImplant base_type;

  Money costToLevel(int curlevel) const;

  IDBImplantAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBImplantAdjust::base_type> {
  typedef IDBImplantAdjust adjusted;
};

struct IDBEffectsAdjust {
  const IDBEffects *idb;
  IDBAdjustment adjust;

public:
  typedef IDBEffects base_type;

  IDBEffects::IDBEType type() const;
  int quantity() const;

  float particle_inertia() const;
  float particle_reflect() const;
  
  float particle_spread() const;

  float particle_slowdown() const;
  float particle_lifetime() const;

  float particle_radius() const;
  Color particle_color() const;
  
  float ionblast_radius() const;
  float ionblast_duration() const;
  
  const vector<pair<int, Color> > &ionblast_visuals() const;

  IDBEffectsAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust);
};
template<> struct IDBItemProperties<IDBEffectsAdjust::base_type> {
  typedef IDBEffectsAdjust adjusted;
};

/*************
 * Hierarchy items
 */

struct HierarchyNode {
public:
  vector<HierarchyNode> branches;

  string name;

  enum Type {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_BOMBARDMENT, HNT_TANK, HNT_IMPLANTSLOT, HNT_IMPLANTITEM, HNT_IMPLANTITEM_UPG, HNT_EQUIP, HNT_EQUIPWEAPON, HNT_SELL, HNT_DONE, HNT_NONE /* for restrictions */, HNT_IMPLANT_CAT /* for restrictions */, HNT_LAST};
  Type type;

  enum Displaymode {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_EQUIP, HNDM_IMPLANT_EQUIP, HNDM_IMPLANT_UPGRADE, HNDM_LAST};
  Displaymode displaymode;

  bool buyable;
  
  int pack;
  
  const IDBWeapon *weapon;
  const IDBUpgrade *upgrade;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBTank *tank;
  
  const IDBImplant *implantitem;
  const IDBImplantSlot *implantslot;
  
  const IDBWeapon *equipweapon;
  
  int cat_restrictiontype;
  Money spawncash;
  
  void checkConsistency(vector<string> *errors) const;
  
  Color getColor() const;
  Color getHighlightColor() const;
  
  HierarchyNode();
  
};

void clearItemdb();

void loadItemdb();
void addItemFile(const string &file);

void reloadItemdb();

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
const map<string, IDBTank> &tankList();
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
