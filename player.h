#ifndef DNET_PLAYER
#define DNET_PLAYER

#include <map>

using namespace std;

#include "itemdb.h"
#include "coord.h"

enum { FACTIONMODE_NONE, FACTIONMODE_MINOR, FACTIONMODE_MEDIUM, FACTIONMODE_MAJOR, FACTIONMODE_LAST };
enum { ITEMSTATE_UNOWNED, ITEMSTATE_BOUGHT, ITEMSTATE_EQUIPPED };

class Player {
public:

  IDBUpgradeAdjust adjustUpgrade(const IDBUpgrade *in_upg) const;
  IDBGloryAdjust adjustGlory(const IDBGlory *in_upg) const;
  IDBBombardmentAdjust adjustBombardment(const IDBBombardment *in_upg) const;
  IDBWeaponAdjust adjustWeapon(const IDBWeapon *in_upg) const;

  bool canBuyUpgrade(const IDBUpgrade *in_upg) const;
  bool canBuyGlory(const IDBGlory *in_glory) const;
  bool canBuyBombardment(const IDBBombardment *in_bombardment) const;
  bool canBuyWeapon(const IDBWeapon *in_weap) const;

  void buyUpgrade(const IDBUpgrade *in_upg);
  void buyGlory(const IDBGlory *in_glory);
  void buyBombardment(const IDBBombardment *in_bombardment);
  void buyWeapon(const IDBWeapon *in_weap);

  // must already be bought
  void equipGlory(const IDBGlory *in_glory);
  void equipBombardment(const IDBBombardment *in_bombardment);
  void equipWeapon(const IDBWeapon *in_weap);

  void sellWeapon(const IDBWeapon *in_weap);
  
  int stateUpgrade(const IDBUpgrade *in_upg) const; // ATM this will only return UNOWNED or EQUIPPED
  int stateGlory(const IDBGlory *in_glory) const;
  int stateBombardment(const IDBBombardment *in_bombardment) const;
  int ammoCount(const IDBWeapon *in_weapon) const;

  const IDBFaction *getFaction() const;
  
  IDBGloryAdjust getGlory() const;
  IDBBombardmentAdjust getBombardment() const;
  IDBTankAdjust getTank() const;

  IDBWeaponAdjust getWeapon() const;
  void cycleWeapon();
  
  Money getCash() const;
  void addCash(Money amount); // this is really designed *solely* for the income phase

  void addKill();
  void addWin();
  void addDamage(float damage);

  int consumeKills();
  int consumeWins();
  float consumeDamage();
  
  float shotFired();
  int shotsLeft() const;

  Player();
  Player(const IDBFaction *fact, int factionmode);

private:
  
  void reCalculate();
  void consumeAmmo(const IDBWeapon *weapon, int count);

  vector<const IDBUpgrade *> upgrades;

  // First item is equipped
  vector<const IDBGlory *> glory;
  vector<const IDBBombardment *> bombardment;

  map<pair<string, const IDBWeapon *>, int> weapons;
  const IDBWeapon *curweapon;

  const IDBFaction *faction;
  int factionmode;

  IDBAdjustment adjustment;

  Money cash;

  float damageDone;
  int kills;
  int wins;
};

#endif
