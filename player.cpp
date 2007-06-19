
#include "player.h"

#include "args.h"

#include <set>

using namespace std;

pair<int, int> Weaponmanager::findWeapon(const IDBWeapon *weap) const {
  pair<int, int> rv = make_pair(-1, -1);
  for(int i = 0; i < weaponops.size(); i++) {
    for(int j = 0; j < weaponops[i].size(); j++) {
      if(weaponops[i][j] == weap) {
        CHECK(rv.first == -1);
        rv.first = i;
        rv.second = j;
      }
    }
  }
  CHECK(rv.first != -1);
  return rv;
}
void Weaponmanager::eraseWeapon(const IDBWeapon *weap) {
  pair<int, int> pos = findWeapon(weap);
  weaponops[pos.first].erase(weaponops[pos.first].begin() + pos.second);
}

float Weaponmanager::shotFired(int id) {
  float val = (float)getWeaponSlot(id)->base_cost.value() / getWeaponSlot(id)->quantity;
  if(ammoCountSlot(id) != UNLIMITED_AMMO)
    removeAmmo(getWeaponSlot(id), 1);
  return val;
}

void Weaponmanager::addAmmo(const IDBWeapon *weap, int count) {
  CHECK(weap);
  CHECK(count > 0);
  if(weapons.count(weap)) {
    weapons[weap] += count;
  } else {
    weapons[weap] = count;
    weaponops[WMSPC_NEW].push_back(weap);
  }
}

void Weaponmanager::removeAmmo(const IDBWeapon *weap, int amount) {
  CHECK(weap);
  CHECK(amount > 0);
  CHECK(weapons.count(weap));
  CHECK(weapons[weap] > 0 && weapons[weap] >= amount);
  if(weapons[weap] == amount) {
    weapons.erase(weap);
    for(int i = 0; i < weaponops.size(); i++)
      if(count(weaponops[i].begin(), weaponops[i].end(), weap))
        weaponops[i].erase(find(weaponops[i].begin(), weaponops[i].end(), weap));
  } else {
    weapons[weap] -= amount;
  }
}

int Weaponmanager::ammoCount(const IDBWeapon *weap) const {
  CHECK(weap);
  if(weap == defaultweapon)
    weap = NULL;
  if(weapons.count(weap))
    return weapons.find(weap)->second;
  return 0;
}

int Weaponmanager::ammoCountSlot(int id) const {
  return ammoCount(getWeaponSlot(id));
}
const IDBWeapon *Weaponmanager::getWeaponSlot(int id) const {
  if(weaponops[id].size())
    return weaponops[id][0];
  CHECK(defaultweapon);
  return defaultweapon;
}

const vector<vector<const IDBWeapon *> > &Weaponmanager::getWeaponList() const {
  return weaponops;
}

void Weaponmanager::moveWeaponUp(const IDBWeapon *a) {
  pair<int, int> weppos = findWeapon(a);
  eraseWeapon(a);
  if(weppos.second == 0) {
    weppos.first = modurot(weppos.first - 1, WMSPC_READY_LAST);
    weppos.second = weaponops[weppos.first].size();
  } else {
    weppos.second--;
  }
  weaponops[weppos.first].insert(weaponops[weppos.first].begin() + weppos.second, a);
}
void Weaponmanager::moveWeaponDown(const IDBWeapon *a) {
  pair<int, int> weppos = findWeapon(a);
  eraseWeapon(a);
  if(weppos.second == weaponops[weppos.first].size()) {
    if(weppos.first == WMSPC_NEW)
      weppos.first = 0;
    else
      weppos.first = modurot(weppos.first + 1, WMSPC_READY_LAST);
    weppos.second = 0;
  } else {
    weppos.second++;
  }
  weaponops[weppos.first].insert(weaponops[weppos.first].begin() + weppos.second, a);
}
void Weaponmanager::promoteWeapon(const IDBWeapon *a, int slot) {
  CHECK(a);
  CHECK(weapons.count(a));
  CHECK(slot >= 0 && slot < SIMUL_WEAPONS);
  eraseWeapon(a);
  weaponops[slot].insert(weaponops[slot].begin(), a);
}
void Weaponmanager::changeDefaultWeapon(const IDBWeapon *weapon) {
  defaultweapon = weapon; // YES. IT'S EASY NOW.
}
bool Weaponmanager::weaponsReady() const {
  return weaponops[WMSPC_NEW].empty();
}

Weaponmanager::Weaponmanager(const IDBWeapon *weapon) {
  defaultweapon = weapon;
  weapons[NULL] = UNLIMITED_AMMO;
  weaponops.resize(WMSPC_LAST);
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
IDBImplantSlotAdjust Player::adjustImplantSlot(const IDBImplantSlot *in_upg) const { return IDBImplantSlotAdjust(in_upg, adjustment); };
IDBImplantAdjust Player::adjustImplant(const IDBImplant *in_upg) const { return IDBImplantAdjust(in_upg, adjustment); };

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) == ITEMSTATE_UNOWNED && adjustUpgradeForCurrentTank(in_upg).cost() <= cash && isUpgradeAvailable(in_upg); }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) == ITEMSTATE_UNOWNED && adjustGlory(in_glory).cost() <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) == ITEMSTATE_UNOWNED && adjustBombardment(in_bombardment).cost() <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const { return adjustWeapon(in_weap).cost(1) <= cash && in_weap->base_cost > Money(0); }
bool Player::canBuyTank(const IDBTank *in_tank) const { return stateTank(in_tank) == ITEMSTATE_UNOWNED && adjustTankWithInstanceUpgrades(in_tank).cost() <= cash; };
bool Player::canBuyImplantSlot(const IDBImplantSlot *in_impslot) const { return stateImplantSlot(in_impslot) == ITEMSTATE_UNOWNED && adjustImplantSlot(in_impslot).cost() <= cash; };

bool Player::isUpgradeAvailable(const IDBUpgrade *in_upg) const {
  if(!tank.size())
    return false;
  
  return count(tank[0].tank->upgrades.begin(), tank[0].tank->upgrades.end(), in_upg->category);
}

Money Player::costWeapon(const IDBWeapon *in_weap) const {
  return adjustWeapon(in_weap).cost_pack();
}

Money Player::costUpgrade(const IDBUpgrade *in_upg) const {
  return adjustUpgradeForCurrentTank(in_upg).cost();
}

Money Player::costGlory(const IDBGlory *in_glory) const {
  return adjustGlory(in_glory).cost();
}

Money Player::costBombardment(const IDBBombardment *in_bombard) const {
  return adjustBombardment(in_bombard).cost();
}

Money Player::costTank(const IDBTank *in_tank) const {
  return adjustTankWithInstanceUpgrades(in_tank).cost();
}

Money Player::costImplantSlot(const IDBImplantSlot *in_slot) const {
  return adjustImplantSlot(in_slot).cost();
}

Money Player::costImplantUpg(const IDBImplant *in_implant) const {
  return adjustImplant(in_implant).costToLevel(implantLevel(in_implant));
}
  
Money Player::sellvalueWeapon(const IDBWeapon *in_weap) const {
  return adjustWeapon(in_weap).sellcost(min(in_weap->quantity, ammoCount(in_weap)));
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
  cash -= adjustTankWithInstanceUpgrades(in_tank).cost();
  tank.push_back(TankEquipment(in_tank));
  equipTank(in_tank);
}
void Player::buyImplantSlot(const IDBImplantSlot *in_impslot) {
  CHECK(cash >= adjustImplantSlot(in_impslot).cost());
  CHECK(canBuyImplantSlot(in_impslot));
  cash -= adjustImplantSlot(in_impslot).cost();
  implantslots.push_back(in_impslot);
};

bool Player::canToggleImplant(const IDBImplant *implant) const {
  return implantequipped.count(implant) || implantequipped.size() < implantslots.size();
}
void Player::toggleImplant(const IDBImplant *implant) {
  CHECK(canToggleImplant(implant));
  if(implantequipped.count(implant)) {
    implantequipped.erase(implant);
  } else {
    implantequipped.insert(implant);
  }
  
  reCalculate();
}
bool Player::hasImplant(const IDBImplant *implant) const {
  return implantequipped.count(implant);
}
int Player::freeImplantSlots() const {
  CHECK(implantequipped.size() <= implantslots.size());
  return implantslots.size() - implantequipped.size();
}

int Player::implantLevel(const IDBImplant *implant) const {
  if(implantlevels.count(implant))
    return implantlevels.find(implant)->second;
  else
    return 1;
}
bool Player::canLevelImplant(const IDBImplant *implant) const {
  return cash >= adjustImplant(implant).costToLevel(implantLevel(implant));
}
void Player::levelImplant(const IDBImplant *implant) {
  CHECK(canLevelImplant(implant));
  cash -= adjustImplant(implant).costToLevel(implantLevel(implant));
  int newlevel = implantLevel(implant) + 1;
  implantlevels[implant] = newlevel;
  
  reCalculate();
}

void Player::forceAcquireWeapon(const IDBWeapon *in_weap, int count) {
  weapons.addAmmo(in_weap, count);
  weapons.promoteWeapon(in_weap, 0);
  CHECK(weapons.getWeaponSlot(0) == in_weap);
  corrupted = true;
}
void Player::forceAcquireUpgrade(const IDBUpgrade *in_upg) {
  CHECK(!hasUpgrade(in_upg));
  CHECK(tank.size() >= 1);
  tank[0].upgrades.push_back(in_upg);
  reCalculate();
  corrupted = true;
}
void Player::forceAcquireBombardment(const IDBBombardment *in_bombard) {
  bombardment.push_back(in_bombard);
  equipBombardment(in_bombard);
  corrupted = true;
}
void Player::forceAcquireGlory(const IDBGlory *in_glory) {
  glory.push_back(in_glory);
  equipGlory(in_glory);
  corrupted = true;
}
void Player::forceAcquireTank(const IDBTank *in_tank) {
  tank.push_back(TankEquipment(in_tank));
  equipTank(in_tank);
  corrupted = true;
}
void Player::forceAcquireImplant(const IDBImplant *in_implant) {
  implantequipped.insert(in_implant);
  reCalculate();
  corrupted = true;
}
void Player::forceLevelImplant(const IDBImplant *in_implant) {
  int newlevel = implantLevel(in_implant) + 1;
  implantlevels[in_implant] = newlevel;
  reCalculate();
  corrupted = true;
}

// Allows you to remove things, even things which are not meant to be removed
void Player::forceRemoveUpgrade(const IDBUpgrade *in_upg) {
  CHECK(hasUpgrade(in_upg));
  CHECK(tank.size() >= 1);
  tank[0].upgrades.erase(find(tank[0].upgrades.begin(), tank[0].upgrades.end(), in_upg));
  reCalculate();
  corrupted = true;
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

void Player::sellWeapon(const IDBWeapon *in_weap) {
  CHECK(weapons.ammoCount(in_weap) > 0);

  int sold = min(weapons.ammoCount(in_weap), in_weap->quantity);
  cash += adjustWeapon(in_weap).sellcost(sold);
  weapons.removeAmmo(in_weap, sold);
}

bool Player::hasUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) != ITEMSTATE_UNOWNED; }
bool Player::hasGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) != ITEMSTATE_UNOWNED; }
bool Player::hasBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) != ITEMSTATE_UNOWNED; }
bool Player::hasTank(const IDBTank *in_tank) const { return stateTank(in_tank) != ITEMSTATE_UNOWNED; }
bool Player::hasImplantSlot(const IDBImplantSlot *in_impslot) const { return stateImplantSlot(in_impslot) != ITEMSTATE_UNOWNED; }

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
int Player::stateImplantSlot(const IDBImplantSlot *in_impslot) const {
  if(count(implantslots.begin(), implantslots.end(), in_impslot))
    return ITEMSTATE_EQUIPPED; // can't be bought without being equipped, heh
  return ITEMSTATE_UNOWNED;
}

vector<string> Player::blockedReasons() const {
  vector<string> rv;
  if(!tank.size())
    rv.push_back("You don't have a tank. You need to buy one before you can enter combat again.");
  if(!weapons.weaponsReady())
    rv.push_back("Not all of your new weapons are equipped. Go equip them first.");
  return rv;
}
bool Player::hasValidTank() const {
  return tank.size(); }

const IDBFaction *Player::getFaction() const {
  return faction; };

void Player::setFactionMode(int faction_mode) {
  factionmode = faction_mode;
  reCalculate();
}

IDBGloryAdjust Player::getGlory() const {
  return adjustGlory(glory[0]); };
IDBBombardmentAdjust Player::getBombardment(int bombard_level) const {
  return adjustBombardment(bombardment[0], bombard_level); };
IDBTankAdjust Player::getTank() const {
  return adjustTankWithInstanceUpgrades(tank[0].tank); };

IDBWeaponAdjust Player::getWeapon(int id) const {
  return adjustWeapon(weapons.getWeaponSlot(id)); };

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
  
const vector<vector<const IDBWeapon *> > &Player::getWeaponList() const {
  return weapons.getWeaponList();
}
void Player::moveWeaponUp(const IDBWeapon *a) {
  weapons.moveWeaponUp(a);
}
void Player::moveWeaponDown(const IDBWeapon *a) {
  weapons.moveWeaponDown(a);
}
void Player::promoteWeapon(const IDBWeapon *a, int slot) {
  weapons.promoteWeapon(a, slot);
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
  
  vector<vector<const IDBWeapon *> > weps = weapons.getWeaponList();
  for(int i = 0; i < weps.size(); i++)
    for(int j = 0; j < weps[i].size(); j++)
      worth += adjustWeapon(weps[i][j]).cost(weapons.ammoCount(weps[i][j]));
  
  for(int i = 0; i < implantslots.size(); i++)
    worth += adjustImplantSlot(implantslots[i]).cost();
  
  for(map<const IDBImplant *, int>::const_iterator itr = implantlevels.begin(); itr != implantlevels.end(); itr++)
    for(int i = 1; i < itr->second; i++)
      worth += adjustImplant(itr->first).costToLevel(i);
  
  return worth;
}

bool Player::isCorrupted() const {
  return corrupted;
}

Player::Player() : weapons(NULL) { // this kind of works with the weapon manager
  cash = Money(-1);
  faction = NULL;
  corrupted = false;
}

Player::Player(const IDBFaction *fact, int in_factionmode, Money money) : weapons(defaultTank()->weapon) {
  faction = fact;
  factionmode = in_factionmode;
  CHECK(factionmode >= 0 && factionmode < FACTIONMODE_LAST);
  cash = money;
  glory.push_back(defaultGlory());
  bombardment.push_back(defaultBombardment());
  tank.push_back(defaultTank());
  reCalculate();
  
  dprintf("----- ADJUSTMENT DATA\n");
  adjustment.debugDump();
  dprintf("----- END ADJUSTMENT DATA\n");
  
  kills = 0;
  wins = 0;
  damageDone = 0;
  corrupted = false;
}

void Player::reCalculate() {
  CHECK(faction);
  
  adjustment = *faction->adjustment[factionmode];
  for(set<const IDBImplant *>::const_iterator itr = implantequipped.begin(); itr != implantequipped.end(); itr++)
    adjustment += (*itr)->makeAdjustment(implantLevel(*itr));
  
  adjustment_notank = adjustment;
  if(tank.size()) {
    for(int i = 0; i < tank[0].upgrades.size(); i++)
      adjustment += *tank[0].upgrades[i]->adjustment;
    if(tank[0].tank->adjustment)
      adjustment += *tank[0].tank->adjustment;
  }
  //adjustment.debugDump();
}
