
#include "player.h"

#include "args.h"

#include <set>

using namespace std;

bool IDBWeaponNameSorter::operator()(const IDBWeapon *lhs, const IDBWeapon *rhs) const {
  if(!lhs && !rhs)
    return false;
  if(!lhs)
    return true;
  if(!rhs)
    return false;
  CHECK(lhs && rhs);
  return lhs->name < rhs->name;
}
  
void Weaponmanager::cycleWeapon(int id) {
  CHECK(weaponops[id].size());
  vector<const IDBWeapon *>::iterator itr = find(weaponops[id].begin(), weaponops[id].end(), curweapons[id]);
  CHECK(itr != weaponops[id].end());
  itr++;
  if(itr == weaponops[id].end()) {
    curweapons[id] = weaponops[id][0];
  } else {
    curweapons[id] = *itr;
  }
}
float Weaponmanager::shotFired(int id) {
  float val = (float)curweapons[id]->base_cost.value() / curweapons[id]->quantity;
  if(ammoCountSlot(id) != UNLIMITED_AMMO)
    removeAmmo(curweapons[id], 1);
  return val;
}

void Weaponmanager::addAmmo(const IDBWeapon *weap, int count) {
  CHECK(count > 0 || count == UNLIMITED_AMMO);
  if(weapons.count(weap)) {
    CHECK(count != UNLIMITED_AMMO);
    weapons[weap] += count;
  } else {
    weapons[weap] = count;
    for(int i = 0; i < SIMUL_WEAPONS; i++)
      setWeaponEquipBit(weap, i, true);
  }
}
void Weaponmanager::removeAmmo(const IDBWeapon *weap, int count) {
  CHECK(count > 0 || count == UNLIMITED_AMMO);
  CHECK(weapons.count(weap));
  CHECK((weapons[weap] > 0 && weapons[weap] >= count) || (weapons[weap] == UNLIMITED_AMMO && count == UNLIMITED_AMMO));
  if(weapons[weap] == count) {
    weapons.erase(weap);
    for(int i = 0; i < weaponops.size(); i++) {
      setWeaponEquipBit(weap, i, false, true);
      CHECK(curweapons[i] != weap);
    }
  } else {
    weapons[weap] -= count;
  }
}

int Weaponmanager::ammoCount(const IDBWeapon *weap) const {
  if(weapons.count(weap))
    return weapons.find(weap)->second;
  return 0;
}

int Weaponmanager::ammoCountSlot(int id) const {
  return ammoCount(curweapons[id]);
}
const IDBWeapon *Weaponmanager::getWeaponSlot(int id) const {
  return curweapons[id];
}

vector<const IDBWeapon *> Weaponmanager::getAvailableWeapons() const {
  set<const IDBWeapon *, IDBWeaponNameSorter> seet;
  for(map<const IDBWeapon *, int, IDBWeaponNameSorter>::const_iterator itr = weapons.begin(); itr != weapons.end(); itr++)
    if(itr->first)
      seet.insert(itr->first);
  return vector<const IDBWeapon *>(seet.begin(), seet.end());
}
void Weaponmanager::setWeaponEquipBit(const IDBWeapon *weapon, int id, bool bit, bool force) {
  if(count(weaponops[id].begin(), weaponops[id].end(), weapon) == bit)
    return;
  if(bit == true) {
    CHECK(weapons.count(weapon));
    weaponops[id].push_back(weapon);
    sort(weaponops[id].begin(), weaponops[id].end(), IDBWeaponNameSorter());
    curweapons[id] = weapon; // equip it
  } else {
    // If we're removing the current weapon, switch to the next weapon first.
    if(curweapons[id] == weapon)
      cycleWeapon(id);
    
    // If we're still removing the current weapon, we only have one weapon.
    if(curweapons[id] == weapon) {
      if(!force)  // Give up, if we're not forcing. If we are . . .
        return;
      
      // If that weapon is our default weapon, something horrible has occured.
      if(curweapons[id] == defaultweapon)
        CHECK(0);
      
      // Otherwise, add the default weapon and then cycle again.
      CHECK(weaponops[id].size() == 1);
      weaponops[id].push_back(defaultweapon);
      cycleWeapon(id);
    }

    // At this point, we must not be removing the current weapon.
    CHECK(curweapons[id] != weapon);
    
    // We're simply removing, so no need to sort.
    // If we added a default weapon, it's now the only item in here, so, again, no need to sort.
    weaponops[id].erase(find(weaponops[id].begin(), weaponops[id].end(), weapon));
  }
}
int Weaponmanager::getWeaponEquipBit(const IDBWeapon *weapon, int id) const {
  return count(weaponops[id].begin(), weaponops[id].end(), weapon) + (curweapons[id] == weapon);
}

void Weaponmanager::changeDefaultWeapon(const IDBWeapon *weapon) {
  StackString sst("changeDefaultWeapon");
  if(weapon == defaultweapon)
    return; // just don't bother
  
  // If the old default weapon is equipped somewhere, switch it with the new default weapon.
  // If the old default weapon is active somewhere, switch it with the new default weapon.
  for(int i = 0; i < curweapons.size(); i++) {
    if(count(weaponops[i].begin(), weaponops[i].end(), defaultweapon)) {
      *find(weaponops[i].begin(), weaponops[i].end(), defaultweapon) = weapon;
      sort(weaponops[i].begin(), weaponops[i].end(), IDBWeaponNameSorter());
      if(curweapons[i] == defaultweapon)
        curweapons[i] = weapon;
    }
    CHECK(getWeaponEquipBit(defaultweapon, i) == WEB_UNEQUIPPED);
  }
  
  weapons.erase(defaultweapon);
  
  CHECK(!weapons.count(defaultweapon));
  CHECK(!weapons.count(weapon));
  
  weapons[weapon] = UNLIMITED_AMMO; // so we don't accidentally equip it somewhere
  
  defaultweapon = weapon;
}

Weaponmanager::Weaponmanager(const IDBWeapon *weapon) {
  defaultweapon = weapon;
  weaponops.resize(SIMUL_WEAPONS, vector<const IDBWeapon*>(1, defaultweapon));
  addAmmo(defaultweapon, UNLIMITED_AMMO);
  curweapons.resize(SIMUL_WEAPONS, defaultweapon);
}

TankEquipment::TankEquipment() { tank = NULL; }
TankEquipment::TankEquipment(const IDBTank *in_tank) { tank = in_tank; }

IDBUpgradeAdjust Player::adjustUpgrade(const IDBUpgrade *in_upg, const IDBTank *in_tank) const {
  return IDBUpgradeAdjust(in_upg, in_tank, adjustment);
}
IDBUpgradeAdjust Player::adjustUpgradeForCurrentTank(const IDBUpgrade *in_upg) const {
  CHECK(tank.size());
  return adjustUpgrade(in_upg, tank[0].tank);
}

IDBGloryAdjust Player::adjustGlory(const IDBGlory *in_upg) const { return IDBGloryAdjust(in_upg, adjustment); };
IDBBombardmentAdjust Player::adjustBombardment(const IDBBombardment *in_upg, int bombard_level) const { return IDBBombardmentAdjust(in_upg, adjustment, bombard_level); };
IDBWeaponAdjust Player::adjustWeapon(const IDBWeapon *in_upg) const { return IDBWeaponAdjust(in_upg, adjustment); };
IDBTankAdjust Player::adjustTankWithInstanceUpgrades(const IDBTank *in_upg) const {
  IDBAdjustment idba = adjustment_notank;
  if(in_upg->adjustment)
    idba += *in_upg->adjustment;
  
  for(int i = 0; i < tank.size(); i++) {
    if(tank[i].tank == in_upg) {
      for(int j = 0; j < tank[i].upgrades.size(); j++)
        idba += *tank[i].upgrades[j]->adjustment;
      return IDBTankAdjust(in_upg, idba);
    }
  }
  
  return IDBTankAdjust(in_upg, idba);
};

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) == ITEMSTATE_UNOWNED && adjustUpgradeForCurrentTank(in_upg).cost() <= cash && isUpgradeAvailable(in_upg); }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) == ITEMSTATE_UNOWNED && adjustGlory(in_glory).cost() <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) == ITEMSTATE_UNOWNED && adjustBombardment(in_bombardment).cost() <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const { return adjustWeapon(in_weap).cost(1) <= cash && in_weap->base_cost > Money(0); }
bool Player::canBuyTank(const IDBTank *in_tank) const { return stateTank(in_tank) == ITEMSTATE_UNOWNED && adjustTankWithInstanceUpgrades(in_tank).cost() <= cash; };

bool Player::isUpgradeAvailable(const IDBUpgrade *in_upg) const {
  if(!tank.size())
    return false;
  
  return count(tank[0].tank->upgrades.begin(), tank[0].tank->upgrades.end(), in_upg->category);
}

bool Player::canSellGlory(const IDBGlory *in_glory) const { return hasGlory(in_glory) && in_glory != defaultGlory(); };
bool Player::canSellBombardment(const IDBBombardment *in_bombardment) const { return hasBombardment(in_bombardment) && in_bombardment != defaultBombardment(); };
bool Player::canSellWeapon(const IDBWeapon *in_weap) const { return ammoCount(in_weap) > 0; }
bool Player::canSellTank(const IDBTank *in_tank) const { return hasTank(in_tank); }  // yes, you can sell all your tanks!

Money Player::sellTankValue(const IDBTank *in_tank) const {
  CHECK(hasTank(in_tank));
  int ps;
  for(ps = 0; ps < tank.size(); ps++)
    if(tank[ps].tank == in_tank)
      break;
  CHECK(ps < tank.size());
  
  Money acu = Money(0);
  acu += adjustTankWithInstanceUpgrades(in_tank).sellcost();
  for(int i = 0; i < tank[ps].upgrades.size(); i++)
    acu += adjustUpgrade(tank[ps].upgrades[i], tank[ps].tank).sellcost();
  return acu;
}

void Player::buyUpgrade(const IDBUpgrade *in_upg) {
  CHECK(cash >= adjustUpgradeForCurrentTank(in_upg).cost());
  CHECK(canBuyUpgrade(in_upg));
  CHECK(tank.size() >= 1);
  cash -= adjustUpgradeForCurrentTank(in_upg).cost();
  tank[0].upgrades.push_back(in_upg);
  reCalculate();
}
void Player::buyGlory(const IDBGlory *in_glory) {
  CHECK(cash >= adjustGlory(in_glory).cost());
  CHECK(canBuyGlory(in_glory));
  cash -= adjustGlory(in_glory).cost();
  glory.push_back(in_glory);
  equipGlory(in_glory);
}
void Player::buyBombardment(const IDBBombardment *in_bombardment) {
  CHECK(cash >= adjustBombardment(in_bombardment).cost());
  CHECK(canBuyBombardment(in_bombardment));
  cash -= adjustBombardment(in_bombardment).cost() ;
  bombardment.push_back(in_bombardment);
  equipBombardment(in_bombardment);
}
void Player::buyWeapon(const IDBWeapon *in_weap) {
  CHECK(canBuyWeapon(in_weap));
  // hahahah awful
  for(int i = in_weap->quantity; i > 0; --i) {
    if(adjustWeapon(in_weap).cost(i) <= cash) {
      weapons.addAmmo(in_weap, i);
      cash -= adjustWeapon(in_weap).cost(i);
      return;
    }
  }
  CHECK(0);
}
void Player::buyTank(const IDBTank *in_tank) {
  CHECK(cash >= adjustTankWithInstanceUpgrades(in_tank).cost());
  CHECK(canBuyTank(in_tank));
  cash -= adjustTankWithInstanceUpgrades(in_tank).cost() ;
  tank.push_back(TankEquipment(in_tank));
  equipTank(in_tank);
}

void Player::forceAcquireWeapon(const IDBWeapon *in_weap, int count) {
  weapons.addAmmo(in_weap, count);
  if(weapons.getWeaponSlot(0) != in_weap) {
    weapons.setWeaponEquipBit(in_weap, 0, false, true);
    weapons.setWeaponEquipBit(in_weap, 0, true, true);
  }
  CHECK(weapons.getWeaponSlot(0) == in_weap);
}
void Player::forceAcquireUpgrade(const IDBUpgrade *in_upg) {
  CHECK(!hasUpgrade(in_upg));
  CHECK(tank.size() >= 1);
  tank[0].upgrades.push_back(in_upg);
  reCalculate();
}
void Player::forceAcquireBombardment(const IDBBombardment *in_bombard) {
  bombardment.push_back(in_bombard);
  equipBombardment(in_bombard);
}
void Player::forceAcquireGlory(const IDBGlory *in_glory) {
  glory.push_back(in_glory);
  equipGlory(in_glory);
}
void Player::forceAcquireTank(const IDBTank *in_tank) {
  tank.push_back(TankEquipment(in_tank));
  equipTank(in_tank);
}

// Allows you to remove things, even things which are not meant to be removed
void Player::forceRemoveUpgrade(const IDBUpgrade *in_upg) {
  CHECK(hasUpgrade(in_upg));
  CHECK(tank.size() >= 1);
  tank[0].upgrades.erase(find(tank[0].upgrades.begin(), tank[0].upgrades.end(), in_upg));
  reCalculate();
}

void Player::equipGlory(const IDBGlory *in_glory) {
  CHECK(count(glory.begin(), glory.end(), in_glory));
  swap(*find(glory.begin(), glory.end(), in_glory), glory[0]);
}
void Player::equipBombardment(const IDBBombardment *in_bombardment) {
  CHECK(count(bombardment.begin(), bombardment.end(), in_bombardment));
  swap(*find(bombardment.begin(), bombardment.end(), in_bombardment), bombardment[0]);
}
void Player::equipTank(const IDBTank *in_tank) {
  // we do *not* bother to check if we already have this tank equipped! If we did that, we might not recalculate or changeDefaultWeapon. Since we might not have any tank before equipping this one (i.e. the player had 0 tanks) we wouldn't equip the default weapon or recalculate properly. This would be bad.
  int ps;
  for(ps = 0; ps < tank.size(); ps++)
    if(tank[ps].tank == in_tank)
      break;
  CHECK(ps < tank.size());
  swap(tank[ps], tank[0]);
  weapons.changeDefaultWeapon(in_tank->weapon);
  reCalculate();
}

void Player::sellGlory(const IDBGlory *in_glory) {
  CHECK(canSellGlory(in_glory));
  glory.erase(find(glory.begin(), glory.end(), in_glory));
  cash += adjustGlory(in_glory).sellcost();
  CHECK(glory.size() != 0);
}
void Player::sellBombardment(const IDBBombardment *in_bombardment) {
  CHECK(canSellBombardment(in_bombardment));
  bombardment.erase(find(bombardment.begin(), bombardment.end(), in_bombardment));
  cash += adjustBombardment(in_bombardment).sellcost();
  CHECK(bombardment.size() != 0);
}
void Player::sellWeapon(const IDBWeapon *in_weap) {
  CHECK(canSellWeapon(in_weap));
  CHECK(weapons.ammoCount(in_weap) > 0);

  int sold = min(weapons.ammoCount(in_weap), in_weap->quantity);
  cash += adjustWeapon(in_weap).sellcost(sold);
  weapons.removeAmmo(in_weap, sold);
}
void Player::sellTank(const IDBTank *in_tank) {
  CHECK(canSellTank(in_tank));
  
  if(tank[0].tank == in_tank && tank.size() >= 2)
    equipTank(tank[1].tank);

  int ps;
  for(ps = 0; ps < tank.size(); ps++)
    if(tank[ps].tank == in_tank)
      break;
  CHECK(ps < tank.size());
  cash += sellTankValue(in_tank);
  tank.erase(tank.begin() + ps);
  weapons.changeDefaultWeapon(NULL);
  reCalculate(); // just in case we got rid of the last tank
}

bool Player::hasUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) != ITEMSTATE_UNOWNED; }
bool Player::hasGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) != ITEMSTATE_UNOWNED; }
bool Player::hasBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) != ITEMSTATE_UNOWNED; }
bool Player::hasTank(const IDBTank *in_tank) const { return stateTank(in_tank) != ITEMSTATE_UNOWNED; }

int Player::stateUpgrade(const IDBUpgrade *in_upg) const {
  if(tank.size() == 0)
    return ITEMSTATE_UNAVAILABLE;
  CHECK(in_upg);
  if(count(tank[0].upgrades.begin(), tank[0].upgrades.end(), in_upg))
    return ITEMSTATE_EQUIPPED; // can't be bought without being equipped, heh
  return ITEMSTATE_UNOWNED;
}
int Player::stateGlory(const IDBGlory *in_glory) const {
  CHECK(glory.size() >= 1);
  if(glory[0] == in_glory)
    return ITEMSTATE_EQUIPPED;
  if(count(glory.begin(), glory.end(), in_glory))
    return ITEMSTATE_BOUGHT;
  return ITEMSTATE_UNOWNED;
}
int Player::stateBombardment(const IDBBombardment *in_bombardment) const {
  CHECK(bombardment.size() >= 1);
  if(bombardment[0] == in_bombardment)
    return ITEMSTATE_EQUIPPED;
  if(count(bombardment.begin(), bombardment.end(), in_bombardment))
    return ITEMSTATE_BOUGHT;
  return ITEMSTATE_UNOWNED;
}
int Player::stateTank(const IDBTank *in_tank) const {
  if(tank.size()) {
    if(tank[0].tank == in_tank)
      return ITEMSTATE_EQUIPPED;
    for(int i = 0; i < tank.size(); i++)
      if(tank[i].tank == in_tank)
        return ITEMSTATE_BOUGHT;
  }
  return ITEMSTATE_UNOWNED;
}

bool Player::canContinue() const {
  return tank.size(); }

const IDBFaction *Player::getFaction() const {
  return faction; };

IDBGloryAdjust Player::getGlory() const {
  return adjustGlory(glory[0]); };
IDBBombardmentAdjust Player::getBombardment(int bombard_level) const {
  return adjustBombardment(bombardment[0], bombard_level); };
IDBTankAdjust Player::getTank() const {
  return adjustTankWithInstanceUpgrades(tank[0].tank); };

IDBWeaponAdjust Player::getWeapon(int id) const {
  return adjustWeapon(weapons.getWeaponSlot(id)); };
void Player::cycleWeapon(int id) {
  weapons.cycleWeapon(id);
}

Money Player::getCash() const {
  return cash;
}
void Player::addCash(Money amount) {
  CHECK(amount >= Money(0));
  dprintf("Adding %s bucks!\n", amount.textual().c_str());
  cash += amount;
}

void Player::accumulateStats(int in_kills, float damage) { damageDone += damage; kills += in_kills; }
void Player::addWin() { wins++; }

int Player::consumeKills() {
  int ki = kills;
  kills = 0;
  return ki;
}
int Player::consumeWins() {
  int wi = wins;
  wins = 0;
  return wi;
}
float Player::consumeDamage() {
  float dd = damageDone;
  damageDone = 0;
  return dd;
}

float Player::shotFired(int id) {
  return weapons.shotFired(id) / adjustment.adjustmentfactor(IDBAdjustment::DISCOUNT_WEAPON);
}
int Player::shotsLeft(int id) const {
  return weapons.ammoCountSlot(id);  
}
int Player::ammoCount(const IDBWeapon *in_weapon) const {
  return weapons.ammoCount(in_weapon);
}

vector<const IDBWeapon *> Player::getAvailableWeapons() const {
  return weapons.getAvailableWeapons();
}
void Player::setWeaponEquipBit(const IDBWeapon *weapon, int id, bool bit) {
  return weapons.setWeaponEquipBit(weapon, id, bit);
}
int Player::getWeaponEquipBit(const IDBWeapon *weapon, int id) const {
  return weapons.getWeaponEquipBit(weapon, id);
}

IDBAdjustment Player::getAdjust() const {
  return adjustment;
}

Money Player::totalValue() const {
  // Let us total the player's net worth.
  Money worth = cash;
  
  for(int i = 0; i < glory.size(); i++)
    worth += adjustGlory(glory[i]).cost();
  
  for(int i = 0; i < bombardment.size(); i++)
    worth += adjustBombardment(bombardment[i]).cost();
  
  for(int i = 0; i < tank.size(); i++) {
    worth += adjustTankWithInstanceUpgrades(tank[i].tank).cost();
    
    for(int j = 0; j < tank[i].upgrades.size(); j++)
      worth += adjustUpgrade(tank[i].upgrades[j], tank[i].tank).cost();
  }
  
  vector<const IDBWeapon *> weps = weapons.getAvailableWeapons();
  for(int i = 0; i < weps.size(); i++)
    worth += adjustWeapon(weps[i]).cost(weapons.ammoCount(weps[i]));
  
  return worth;
}

Player::Player() : weapons(NULL) { // this kind of works with the weapon manager
  cash = Money(-1);
  faction = NULL;
}

Player::Player(const IDBFaction *fact, int in_factionmode, Money money) : weapons(defaultTank()->weapon) {
  faction = fact;
  factionmode = in_factionmode;
  CHECK(factionmode >= 0 && factionmode < FACTIONMODE_LAST);
  CHECK(factionmode == 0);
  cash = money;
  glory.push_back(defaultGlory());
  bombardment.push_back(defaultBombardment());
  tank.push_back(defaultTank());
  reCalculate();
  
  kills = 0;
  wins = 0;
  damageDone = 0;
}

void Player::reCalculate() {
  CHECK(faction);
  adjustment = *faction->adjustment[factionmode];
  adjustment_notank = *faction->adjustment[factionmode];
  if(tank.size()) {
    for(int i = 0; i < tank[0].upgrades.size(); i++)
      adjustment += *tank[0].upgrades[i]->adjustment;
    if(tank[0].tank->adjustment)
      adjustment += *tank[0].tank->adjustment;
  }
  //adjustment.debugDump();
}
