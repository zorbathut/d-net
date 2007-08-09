#ifndef DNET_PLAYER
#define DNET_PLAYER

#include "itemdb.h"

#include <map>
#include <set>

using namespace std;

enum { FACTIONMODE_NONE, FACTIONMODE_MINOR, FACTIONMODE_MEDIUM, FACTIONMODE_MAJOR, FACTIONMODE_LAST };
enum { ITEMSTATE_UNOWNED, ITEMSTATE_BOUGHT, ITEMSTATE_EQUIPPED, ITEMSTATE_UNAVAILABLE };
enum { WEB_UNEQUIPPED, WEB_EQUIPPED, WEB_ACTIVE };

class IDBWeaponNameSorter {
public:
  bool operator()(const IDBWeapon *lhs, const IDBWeapon *rhs) const;
};

enum { WMSPC_UNEQUIPPED = SIMUL_WEAPONS, WMSPC_NEW, WMSPC_LAST, WMSPC_READY_LAST = WMSPC_UNEQUIPPED + 1 };

const int UNLIMITED_AMMO = -1938;

class Weaponmanager {
private:
  map<const IDBWeapon *, int> weapons;
  vector<vector<const IDBWeapon *> > weaponops;

  const IDBWeapon *defaultweapon;

  pair<int, int> findWeapon(const IDBWeapon *weap) const;
  void eraseWeapon(const IDBWeapon *weap);
  
public:
  void shotFired(int id);  // Fire a single shot.

  void addAmmo(const IDBWeapon *weap, int count);
  void removeAmmo(const IDBWeapon *weap, int count);

  int ammoCount(const IDBWeapon *weap) const;
  
  int ammoCountSlot(int id) const;  
  const IDBWeapon *getWeaponSlot(int id) const;
  
  const vector<vector<const IDBWeapon *> > &getWeaponList() const;

  void moveWeaponUp(const IDBWeapon *a);
  void moveWeaponDown(const IDBWeapon *a);
  void promoteWeapon(const IDBWeapon *a, int slot);
  void changeDefaultWeapon(const IDBWeapon *weapon);
  bool weaponsReady() const;

  Weaponmanager(const IDBWeapon *weapon);
};

class TankEquipment {
public:
  const IDBTank *tank;
  vector<const IDBUpgrade *> upgrades;

  TankEquipment();
  TankEquipment(const IDBTank *tank);
};

class Player {
public:

  IDBUpgradeAdjust adjustUpgrade(const IDBUpgrade *in_upg, const IDBTank *in_tank) const;
  IDBUpgradeAdjust adjustUpgradeForCurrentTank(const IDBUpgrade *in_upg) const;
  IDBGloryAdjust adjustGlory(const IDBGlory *in_upg) const;
  IDBBombardmentAdjust adjustBombardment(const IDBBombardment *in_upg, int bombard_level = -1) const;
  IDBWeaponAdjust adjustWeapon(const IDBWeapon *in_upg) const;
  IDBTankAdjust adjustTankWithInstanceUpgrades(const IDBTank *in_upg) const;
  IDBImplantSlotAdjust adjustImplantSlot(const IDBImplantSlot *in_impslot) const;
  IDBImplantAdjust adjustImplant(const IDBImplant *in_imp) const;

  bool canBuyUpgrade(const IDBUpgrade *in_upg) const;
  bool canBuyGlory(const IDBGlory *in_glory) const;
  bool canBuyBombardment(const IDBBombardment *in_bombardment) const;
  bool canBuyWeapon(const IDBWeapon *in_weap) const;
  bool canBuyTank(const IDBTank *in_tank) const;
  bool canBuyImplantSlot(const IDBImplantSlot *in_impslot) const;

  bool isUpgradeAvailable(const IDBUpgrade *in_upg) const;
  
  Money costWeapon(const IDBWeapon *in_weap) const;
  Money costUpgrade(const IDBUpgrade *in_upg) const;
  Money costGlory(const IDBGlory *in_glory) const;
  Money costBombardment(const IDBBombardment *in_bombard) const;
  Money costTank(const IDBTank *in_tank) const;
  Money costImplantSlot(const IDBImplantSlot *in_slot) const;
  Money costImplantUpg(const IDBImplant *in_implant) const;
  
  Money sellvalueWeapon(const IDBWeapon *in_weap) const;

  void buyUpgrade(const IDBUpgrade *in_upg);
  void buyGlory(const IDBGlory *in_glory);
  void buyBombardment(const IDBBombardment *in_bombardment);
  void buyWeapon(const IDBWeapon *in_weap);
  void buyTank(const IDBTank *in_tank);
  void buyImplantSlot(const IDBImplantSlot *in_impslot);
  
  // Implant stuff, which is pretty fundamentally different
  bool canToggleImplant(const IDBImplant *implant) const;
  void toggleImplant(const IDBImplant *implant);
  bool hasImplant(const IDBImplant *implant) const;
  int freeImplantSlots() const;
  
  int implantLevel(const IDBImplant *implant) const;
  bool canLevelImplant(const IDBImplant *implant) const;
  void levelImplant(const IDBImplant *implant);

  // Allows you to acquire things. Does not do sanity checks. Should not be used for anything involving meaningful game logic!
  void forceAcquireWeapon(const IDBWeapon *in_weap, int count);
  void forceAcquireUpgrade(const IDBUpgrade *in_upg);
  void forceAcquireBombardment(const IDBBombardment *in_bombard);
  void forceAcquireGlory(const IDBGlory *in_glory);
  void forceAcquireTank(const IDBTank *in_tank);
  void forceAcquireImplant(const IDBImplant *in_implant);
  void forceLevelImplant(const IDBImplant *in_implant);
  
  // Allows you to remove things, even things which are not meant to be removed
  void forceRemoveUpgrade(const IDBUpgrade *in_upg);

  // must already be bought
  void equipGlory(const IDBGlory *in_glory);
  void equipBombardment(const IDBBombardment *in_bombardment);
  void equipTank(const IDBTank *in_tank);

  void sellWeapon(const IDBWeapon *in_weap);
  
  bool hasUpgrade(const IDBUpgrade *in_upg) const;
  bool hasGlory(const IDBGlory *in_glory) const;
  bool hasBombardment(const IDBBombardment *in_bombardment) const;
  bool hasTank(const IDBTank *in_tank) const;
  bool hasImplantSlot(const IDBImplantSlot *in_impslot) const;
  
  int stateUpgrade(const IDBUpgrade *in_upg) const; // ATM this will only return UNOWNED or EQUIPPED
  int stateImplantSlot(const IDBImplantSlot *in_impslot) const; // Same
  int stateGlory(const IDBGlory *in_glory) const;
  int stateBombardment(const IDBBombardment *in_bombardment) const;
  int stateTank(const IDBTank *in_tank) const;
  
  vector<string> blockedReasons() const;
  bool hasValidTank() const;

  const IDBFaction *getFaction() const;
  void setFactionMode(int faction_mode);
  
  IDBGloryAdjust getGlory() const;
  IDBBombardmentAdjust getBombardment(int bombard_level) const;
  IDBTankAdjust getTank() const;
  
  Money getCash() const;
  void addCash(Money amount); // this is really designed *solely* for the income phase

  void accumulateStats(int kills, float damage);
  void addWin();

  int consumeKills();
  int consumeWins();
  float consumeDamage();

  IDBWeaponAdjust getWeapon(int id) const;
  void shotFired(int id);  // Fire a single shot.
  int shotsLeft(int id) const;
  int ammoCount(const IDBWeapon *in_weapon) const;
  
  const vector<vector<const IDBWeapon *> > &getWeaponList() const;
  void moveWeaponUp(const IDBWeapon *a);
  void moveWeaponDown(const IDBWeapon *a);
  void promoteWeapon(const IDBWeapon *a, int slot);
  
  IDBAdjustment getAdjust() const;
  
  Money totalValue() const; // Update this when more stuff is added to the player
  
  bool isCorrupted() const;

  Player();
  Player(const IDBFaction *fact, int factionmode, Money cash);

private:
  
  bool corrupted; // set on any of the "force" functions
  
  void reCalculate();

  Weaponmanager weapons;

  // First item is equipped
  vector<const IDBGlory *> glory;
  vector<const IDBBombardment *> bombardment;
  vector<TankEquipment> tank;

  vector<const IDBImplantSlot *> implantslots;

  map<const IDBImplant *, int> implantlevels;
  set<const IDBImplant *> implantequipped;

  const IDBFaction *faction;
  int factionmode;

  IDBAdjustment adjustment;
  IDBAdjustment adjustment_notank;

  Money cash;

  float damageDone;
  int kills;
  int wins;
};

#endif
