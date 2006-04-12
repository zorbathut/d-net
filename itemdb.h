#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "gfx.h"

#include <string>
#include <vector>

using namespace std;

class Player;
  
struct IDBAdjustment {
public:
  int damage_proj;
  int damage_snipe;
  int damage_explode;
  int damage_trap;
  int damage_exotic;
  int discount_weapon;
  int discount_training;
  int discount_upgrade;
  int discount_license;
  int discount_tank;
  int waste_reduction;
  int tank_firerate;
  int tank_speed;
  int tank_turn;
  int tank_armor;

  IDBAdjustment();
};

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
  int hull;
  int engine;
  int handling;

  Money base_cost;
};

struct IDBBombardment {
public:
  const IDBWarhead *warhead;

  int lockdelay;
  int unlockdelay;

  Money base_cost;
};

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

const HierarchyNode &itemDbRoot();

const IDBWeapon *defaultWeapon();
const IDBGlory *defaultGlory();
const IDBBombardment *defaultBombardment();
const vector<IDBFaction> &factionList();

#endif
