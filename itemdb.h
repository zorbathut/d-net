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

const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "warhead_radius_falloff", "tank_firerate", "tank_speed", "tank_turn", "tank_armor", "bombardment_speed", "discount_weapon", "discount_implant", "discount_upgrade", "discount_tank", "recycle_bonus", "damage_all", "all" };
const bool adjust_negative[] = { false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false };
const int adjust_category[] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
const char * const adjust_human[] = {"Kinetic weapons", "Energy weapons", "Explosive weapons", "Trap weapons", "Exotic weapons", "Blast radius", "Tank firerate", "Tank speed", "Tank turning", "Tank armor", "Bombardment speed", "Weapon discount", "Implant discount", "Upgrade discount", "Tank discount", "Sell efficiency", "All weapons", "Everything" };
const char * const adjust_unit[] = {" KPE", " KJE", " TOTE", " FSE", " flux", " m", "", " m/s", " rad/s", " CME", "", "", "", "", "", ""};

pair<string, bool> adjust_modifiertext(int id, int amount);

struct IDBAdjustment {
public:
  // NOTE: there is currently an implicit assumption that the "damage" stats are first. Don't violate this. Look for DAMAGE_* for the places where it matters.
  // Also, these are obviously correlated with the string arrays above.
  enum IDBAType { DAMAGE_KINETIC, DAMAGE_ENERGY, DAMAGE_EXPLOSIVE, DAMAGE_TRAP, DAMAGE_EXOTIC, WARHEAD_RADIUS_FALLOFF, TANK_FIRERATE, TANK_SPEED, TANK_TURN, TANK_ARMOR, BOMBARDMENT_SPEED, DISCOUNT_WEAPON, DISCOUNT_IMPLANT, DISCOUNT_UPGRADE, DISCOUNT_TANK, RECYCLE_BONUS, LAST, DAMAGE_ALL = LAST, ALL, COMBO_LAST, DAMAGE_LAST = DAMAGE_EXOTIC + 1, DISCOUNT_BEGIN = DISCOUNT_WEAPON, DISCOUNT_END = DISCOUNT_TANK + 1, TANK_BEGIN = TANK_FIRERATE, TANK_END = TANK_ARMOR + 1 };
  
  bool ignore_excessive_radius;
  int adjusts[LAST];  // These are in percentage points away from 100. Yes, this is kind of weird.
  
  int tankhpboost;
  int tankspeedreduction;
  
  pair<int, int> adjustlist[5];
  
  double adjustmentfactor(IDBAType type) const;
  double recyclevalue() const; // this is annoyingly different
  double tankhp() const; // this is also annoyingly different
  double tankspeed() const; // this is *also* annoyingly different. I should wrap these up into DisplayAdjustment and RealAdjustment

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

  bool particle_distribute;
  
  // percentage
  float particle_multiple_inertia;
  float particle_multiple_reflect;
  float particle_multiple_force;   // for "outside forces"
  
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
  float radiusexplosive; // how much of the radius is affected by "explosive bonus"
  Color radiuscolor_bright;
  Color radiuscolor_dim;

  float wallremovalradius;
  float wallremovalchance;

  vector<const IDBDeploy *> deploy;

  vector<const IDBEffects *> effects_impact;
};

enum IDBPMotion { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE, PM_BOOMERANG, PM_MINE, PM_SPIDERMINE, PM_HUNTER, PM_DPS, PM_DELAY, PM_LAST };
enum IDBPShape { PS_LINE, PS_LINE_AIRBRAKE, PS_ARROW, PS_DRONE, PS_STAR, PS_INVISIBLE };

struct IDBProjectile {
  IDBPMotion motion;
    float velocity;
    float proximity;
    float durability;
    float halflife;
    bool penetrating;
    
    float missile_sidelaunch;
    float missile_backlaunch;
    float missile_stabstart;
    float missile_stabilization;
    
    float boomerang_convergence;
    float boomerang_intersection;
    float boomerang_maxrotate;
    
    float airbrake_life;
    float airbrake_slowdown;
    
    float hunter_rotation;
    float hunter_turnweight;
    
    float dps_duration;
    vector<const IDBWarhead *> dps_instant_warhead;
    vector<pair<int, Color> > dps_visuals;
    
    float delay_duration;
  
  IDBPShape shape;
    float proximity_visibility;
    
    float line_length;
    
    float arrow_width;
    float arrow_height;
    float arrow_rotate;
    
    float drone_radius;
    float drone_spike;
    
    int star_spikes;
    float star_radius;
    
    float visual_thickness;
    Color color;
  
  bool no_intersection;

  vector<const IDBWarhead *> chain_warhead;
  vector<const IDBDeploy *> chain_deploy;
  
  vector<const IDBEffects *> chain_effects;
  vector<const IDBEffects *> burn_effects;
};

// Normal specifies "Forward" for tanks, or "Centroid" on cases where there is no tank
enum IDBDType { DT_NORMAL, DT_FORWARD, DT_REAR, DT_CENTROID, DT_MINEPATH, DT_DIRECTED, DT_EXPLODE, DT_LAST };

struct IDBDeploy {
  IDBDType type;

  float anglemodifier;
  float anglestddev;

  int exp_minsplits;
  int exp_maxsplits;
  int exp_minsplitsize;
  int exp_maxsplitsize;

  int exp_shotspersplit;
  
  float directed_range;
  float directed_approach;

  vector<const IDBDeploy *> chain_deploy;
  vector<const IDBProjectile *> chain_projectile;
  vector<const IDBWarhead *> chain_warhead;
  vector<const IDBEffects *> chain_effects;
};

enum IDBWDemoMode { WDM_FIRINGRANGE, WDM_BACKRANGE, WDM_MINES, WDM_LAST };
enum IDBWFiringRangeDistance { WFRD_NORMAL, WFRD_MELEE };
struct IDBLauncher {
  const IDBDeploy *deploy;

  IDBWDemoMode demomode;
  IDBWFiringRangeDistance firingrange_distance;
  
  const string *text;
};

struct IDBWeapon {
  float firerate;
  const IDBLauncher *launcher;

  Money base_cost;
  int quantity;
  int recommended;
  
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

  double costmult;
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
  
  vector<Coord2> getTankVertices(const CPosInfo &cpi) const;
  
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
  Coord mf;
  
  float accumulate(const float *damage) const;
  
public:
  typedef IDBWarhead base_type;

  Coord impactdamage() const;

  Coord radiusdamage() const;
  Coord radiusfalloff() const;
  Color radiuscolor_bright() const;
  Color radiuscolor_dim() const;
  
  Coord wallremovalradius() const;
  Coord wallremovalchance() const;

  vector<IDBDeployAdjust> deploy() const;
  vector<IDBEffectsAdjust> effects_impact() const;
  
  float stats_damagePerShot() const;
  float stats_damagePerShotType(int type) const;

  const IDBWarhead *base() const;
  Coord multfactor() const;
  IDBWarheadAdjust multiply(Coord mult) const;
  
  void checksum(Adler32 *adl) const;

  IDBWarheadAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust, Coord in_multfactor = 1.0);
};
template<> struct IDBItemProperties<IDBWarheadAdjust::base_type> {
  typedef IDBWarheadAdjust adjusted;
};

struct IDBProjectileAdjust {
  const IDBProjectile *idb;
  IDBAdjustment adjust;
  
public:
  typedef IDBProjectile base_type;

  IDBPMotion motion() const;
    float velocity() const;
    float proximity() const;
    float durability() const;
    float halflife() const;
    bool penetrating() const;
    
    float missile_stabstart() const;
    float missile_stabilization() const;
    float missile_sidelaunch() const;
    float missile_backlaunch() const;
    
    float boomerang_convergence() const;
    float boomerang_intersection() const;
    float boomerang_maxrotate() const;
    
    float airbrake_life() const;
    float airbrake_slowdown() const;
    
    float hunter_rotation() const;
    float hunter_turnweight() const;
    
    float dps_duration() const;
    vector<IDBWarheadAdjust> dps_instant_warhead() const;
    const vector<pair<int, Color> > &dps_visuals() const;
    
    float delay_duration() const;
  
  IDBPShape shape() const;
    float proximity_visibility() const;
    
    float line_length() const;
    
    float arrow_width() const;
    float arrow_height() const;
    float arrow_rotate() const;
    
    float drone_radius() const;
    float drone_spike() const;
    
    int star_spikes() const;
    float star_radius() const;
    
    float visual_thickness() const;
    Color color() const;
  
  bool no_intersection() const;
  
  vector<IDBWarheadAdjust> chain_warhead(float multfactor = 1.0f) const;
  vector<IDBDeployAdjust> chain_deploy() const;
  vector<IDBEffectsAdjust> chain_effects() const;
  
  vector<IDBEffectsAdjust> burn_effects() const;
  
  float stats_damagePerShot() const;
  
  void checksum(Adler32 *adl) const;

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

  IDBDType type() const;

  int exp_minsplits() const;
  int exp_maxsplits() const;

  int exp_minsplitsize() const;
  int exp_maxsplitsize() const;

  int exp_shotspersplit() const;

  float anglestddev() const;
  float anglemodifier() const;

  float directed_range() const;
  float directed_approach() const;
  
  vector<IDBDeployAdjust> chain_deploy() const;
  vector<IDBProjectileAdjust> chain_projectile() const;
  vector<IDBWarheadAdjust> chain_warhead() const;
  vector<IDBEffectsAdjust> chain_effects() const;

  float stats_damagePerShot() const;
  
  void checksum(Adler32 *adl) const;

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

  void checksum(Adler32 *adl) const;

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

  IDBLauncherAdjust launcher() const;
  
  pair<int, int> simulateWeaponFiring(Rng *rng) const;

  float firerate() const;
  Money cost(int shots) const;
  Money cost_pack() const;
  Money sellcost(int shots) const;

  bool glory_resistance() const;

  float stats_damagePerSecond() const;
  float stats_damagePerSecondType(int type) const;

  float stats_costPerDamage() const;
  float stats_costPerSecond() const;

  const base_type *base() const;
  void checksum(Adler32 *adl) const;
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
  
void checksum(Adler32 *adl) const;
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

  void checksum(Adler32 *adl) const;
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

  void checksum(Adler32 *adl) const;
  IDBBombardmentAdjust(const base_type *in_idb, const IDBAdjustment &in_adjust, Coord blevel);
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
  vector<Coord2> getTankVertices(const CPosInfo &cpi) const;

  Money cost() const;
  Money sellcost() const;

  const IDBTank *base() const;
  void checksum(Adler32 *adl) const;
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

  void checksum(Adler32 *adl) const;
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

  void checksum(Adler32 *adl) const;
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

  bool particle_distribute() const;

  float particle_multiple_inertia() const;
  float particle_multiple_reflect() const;
  float particle_multiple_force() const;
  
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

  enum Type {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_BOMBARDMENT, HNT_TANK, HNT_IMPLANTSLOT, HNT_IMPLANTITEM, HNT_IMPLANTITEM_EQUIP, HNT_IMPLANTITEM_UPG, HNT_EQUIP, HNT_EQUIPWEAPON, HNT_EQUIPCATEGORY, HNT_SELL, HNT_SELLWEAPON, HNT_BONUSES, HNT_DONE, HNT_NONE /* for restrictions */, HNT_IMPLANT_CAT, HNT_EQUIP_CAT /* for restrictions */, HNT_LAST};
  Type type;

  enum Displaymode {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_EQUIP, HNDM_IMPLANT_EQUIP, HNDM_IMPLANT_UPGRADE, HNDM_LAST};
  Displaymode displaymode;

  bool buyable;
  bool selectable;
  
  int pack;
  
  const IDBWeapon *weapon;
  const IDBUpgrade *upgrade;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBTank *tank;
  
  const IDBImplant *implantitem;
  const IDBImplantSlot *implantslot;
  
  const IDBWeapon *equipweapon;
  bool equipweaponfirst;
  
  const IDBWeapon *sellweapon;
  
  int cat_restrictiontype;
  Money spawncash;
  Money despawncash;
  
  void finalSort();
  void checkConsistency(vector<string> *errors) const;
  
  Color getColor() const;
  Color getHighlightColor() const;
  
  HierarchyNode();
  
};

void adler(Adler32 *adl, const HierarchyNode &idb);

void swap(HierarchyNode &lhs, HierarchyNode &rhs);

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
const map<string, IDBUpgrade> &upgradeList();
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
const string &nameFromIDB(const IDBTank *tank);
const string &nameFromIDB(const IDBProjectile *tank);
const string &nameFromIDB(const IDBUpgrade *tank);
const string &nameFromIDB(const IDBImplant *tank);
const string &nameFromIDB(const IDBImplantSlot *tank);

string informalNameFromIDB(const IDBWeapon *idbw);
string informalNameFromIDB(const IDBWeaponAdjust &idbwa);

void adler(Adler32 *adl, const IDBFaction *idb);

void adler(Adler32 *adl, const IDBWeapon *idb);
void adler(Adler32 *adl, const IDBBombardment *idb);
void adler(Adler32 *adl, const IDBGlory *idb);
void adler(Adler32 *adl, const IDBTank *idb);
void adler(Adler32 *adl, const IDBProjectile *idb);
void adler(Adler32 *adl, const IDBUpgrade *idb);
void adler(Adler32 *adl, const IDBImplant *idb);
void adler(Adler32 *adl, const IDBImplantSlot *idb);

void adler(Adler32 *adl, const IDBAdjustment &idb);

void adler(Adler32 *adl, const IDBWeaponAdjust &idb);
void adler(Adler32 *adl, const IDBBombardmentAdjust &idb);
void adler(Adler32 *adl, const IDBGloryAdjust &idb);
void adler(Adler32 *adl, const IDBTankAdjust &idb);
void adler(Adler32 *adl, const IDBUpgradeAdjust &idb);
void adler(Adler32 *adl, const IDBProjectileAdjust &idb);

#endif
