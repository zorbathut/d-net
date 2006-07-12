#ifndef DNET_PLAYER
#define DNET_PLAYER

#include "itemdb.h"

#include <map>

using namespace std;

enum { FACTIONMODE_NONE, FACTIONMODE_MINOR, FACTIONMODE_MEDIUM, FACTIONMODE_MAJOR, FACTIONMODE_LAST };
enum { ITEMSTATE_UNOWNED, ITEMSTATE_BOUGHT, ITEMSTATE_EQUIPPED, ITEMSTATE_UNAVAILABLE };
enum { WEB_UNEQUIPPED, WEB_EQUIPPED, WEB_ACTIVE };

class IDBWeaponNameSorter {
public:
  bool operator()(const IDBWeapon *lhs, const IDBWeapon *rhs) const;
};

class Weaponmanager {
private:
  map<const IDBWeapon *, int, IDBWeaponNameSorter> weapons;
  vector<vector<const IDBWeapon *> > weaponops;
  vector<const IDBWeapon *> curweapons;

  const IDBWeapon *defaultweapon;
  
public:
  void cycleWeapon(int id);
  float shotFired(int id);  // Fire a single shot. Returns the cost of that shot. This could likely be done better.

  void addAmmo(const IDBWeapon *weap, int count);
  void removeAmmo(const IDBWeapon *weap, int count);

  int ammoCount(const IDBWeapon *weap) const;
  
  int ammoCountSlot(int id) const;  
  const IDBWeapon *getWeaponSlot(int id) const;
  
  vector<const IDBWeapon *> getAvailableWeapons() const;
  void setWeaponEquipBit(const IDBWeapon *weapon, int id, bool bit);
  int getWeaponEquipBit(const IDBWeapon *weapon, int id) const;

  void changeDefaultWeapon(const IDBWeapon *weapon);

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

  IDBUpgradeAdjust adjustUpgrade(const IDBUpgrade *in_upg) const;
  IDBGloryAdjust adjustGlory(const IDBGlory *in_upg) const;
  IDBBombardmentAdjust adjustBombardment(const IDBBombardment *in_upg) const;
  IDBWeaponAdjust adjustWeapon(const IDBWeapon *in_upg) const;
  IDBTankAdjust adjustTank(const IDBTank *in_upg) const;

  bool canBuyUpgrade(const IDBUpgrade *in_upg) const;
  bool canBuyGlory(const IDBGlory *in_glory) const;
  bool canBuyBombardment(const IDBBombardment *in_bombardment) const;
  bool canBuyWeapon(const IDBWeapon *in_weap) const;
  bool canBuyTank(const IDBTank *in_tank) const;

  bool canSellGlory(const IDBGlory *in_glory) const;
  bool canSellBombardment(const IDBBombardment *in_bombardment) const;
  bool canSellWeapon(const IDBWeapon *in_weap) const;
  bool canSellTank(const IDBTank *in_tank) const;
  
  Money sellTankValue(const IDBTank *in_tank) const;

  void buyUpgrade(const IDBUpgrade *in_upg);
  void buyGlory(const IDBGlory *in_glory);
  void buyBombardment(const IDBBombardment *in_bombardment);
  void buyWeapon(const IDBWeapon *in_weap);
  void buyTank(const IDBTank *in_tank);

  // Allows you to acquire things. Does not do sanity checks. Should not be used for anything involving meaningful game logic!
  void forceAcquireWeapon(const IDBWeapon *in_weap, int count);
  void forceAcquireUpgrade(const IDBUpgrade *in_upg);
  void forceAcquireBombardment(const IDBBombardment *in_bombard);
  void forceAcquireGlory(const IDBGlory *in_bombard);
  void forceAcquireTank(const IDBTank *in_bombard);
  
  // Allows you to remove things, even things which are not meant to be removed
  void forceRemoveUpgrade(const IDBUpgrade *in_upg);

  // must already be bought
  void equipGlory(const IDBGlory *in_glory);
  void equipBombardment(const IDBBombardment *in_bombardment);
  void equipTank(const IDBTank *in_tank);

  void sellGlory(const IDBGlory *in_glory);
  void sellBombardment(const IDBBombardment *in_bombardment);
  void sellWeapon(const IDBWeapon *in_weap);
  void sellTank(const IDBTank *in_tank);
  
  bool hasUpgrade(const IDBUpgrade *in_upg) const; // ATM this will only return UNOWNED or EQUIPPED
  bool hasGlory(const IDBGlory *in_glory) const;
  bool hasBombardment(const IDBBombardment *in_bombardment) const;
  bool hasTank(const IDBTank *in_tank) const;
  
  int stateUpgrade(const IDBUpgrade *in_upg) const; // ATM this will only return UNOWNED or EQUIPPED
  int stateGlory(const IDBGlory *in_glory) const;
  int stateBombardment(const IDBBombardment *in_bombardment) const;
  int stateTank(const IDBTank *in_tank) const;
  
  bool canContinue() const;

  const IDBFaction *getFaction() const;
  
  IDBGloryAdjust getGlory() const;
  IDBBombardmentAdjust getBombardment() const;
  IDBTankAdjust getTank() const;
  
  Money getCash() const;
  void addCash(Money amount); // this is really designed *solely* for the income phase

  void addKill();
  void addWin();
  void addDamage(float damage);

  int consumeKills();
  int consumeWins();
  float consumeDamage();

  IDBWeaponAdjust getWeapon(int id) const;
  void cycleWeapon(int id);
  float shotFired(int id);  // Fire a single shot. Returns the cost of that shot. This could likely be done better.
  int shotsLeft(int id) const;
  int ammoCount(const IDBWeapon *in_weapon) const;
  
  vector<const IDBWeapon *> getAvailableWeapons() const;
  void setWeaponEquipBit(const IDBWeapon *weapon, int id, bool bit);
  int getWeaponEquipBit(const IDBWeapon *weapon, int id) const;
  
  IDBAdjustment getAdjust() const;

  Player();
  Player(const IDBFaction *fact, int factionmode);

private:
  
  void reCalculate();

  Weaponmanager weapons;

  // First item is equipped
  vector<const IDBGlory *> glory;
  vector<const IDBBombardment *> bombardment;
  vector<TankEquipment> tank;

  const IDBFaction *faction;
  int factionmode;

  IDBAdjustment adjustment;

  Money cash;

  float damageDone;
  int kills;
  int wins;
};

#endif
