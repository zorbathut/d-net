
#include "player.h"

#include "args.h"
#include "const.h"

#include <set>

using namespace std;

DEFINE_int(startingCash, 1000, "Cash to start with");

bool IDBWeaponNameSorter::operator()(const IDBWeapon *lhs, const IDBWeapon *rhs) const {
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
  float val = curweapons[id]->base_cost.toFloat() / curweapons[id]->quantity;
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
    setWeaponEquipBit(weap, 0, true);
  }
}
void Weaponmanager::removeAmmo(const IDBWeapon *weap, int count) {
  CHECK(count > 0);
  CHECK(weapons.count(weap));
  CHECK(weapons[weap] > 0 && weapons[weap] >= count);
  if(weapons[weap] == count) {
    weapons.erase(weap);
    for(int i = 0; i < weaponops.size(); i++) {
      setWeaponEquipBit(weap, i, false);
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
  for(int i = 0; i < weaponops.size(); i++)
    seet.insert(weaponops[i].begin(), weaponops[i].end());
  return vector<const IDBWeapon *>(seet.begin(), seet.end());
}
void Weaponmanager::setWeaponEquipBit(const IDBWeapon *weapon, int id, bool bit) {
  if(count(weaponops[id].begin(), weaponops[id].end(), weapon) == bit)
    return;
  if(bit == true) {
    CHECK(weapons.count(weapon));
    weaponops[id].push_back(weapon);
    sort(weaponops[id].begin(), weaponops[id].end());
    curweapons[id] = weapon; // equip it
  } else {
    if(curweapons[id] == weapon)
      cycleWeapon(id);
    if(curweapons[id] == weapon) {
      CHECK(weaponops[id].size() == 1);
      weaponops[id].push_back(defaultWeapon());
      cycleWeapon(id);
    }
    CHECK(curweapons[id] != weapon);
    
    // We're simply removing, so no need to sort.
    // If we added a default weapon, it's now the only item in here, so, again, no need to sort.
    weaponops[id].erase(find(weaponops[id].begin(), weaponops[id].end(), weapon));
  }
}
bool Weaponmanager::getWeaponEquipBit(const IDBWeapon *weapon, int id) const {
  return count(weaponops[id].begin(), weaponops[id].end(), weapon);
}

Weaponmanager::Weaponmanager() {
  weaponops.resize(SIMUL_WEAPONS, vector<const IDBWeapon*>(1, defaultWeapon()));
  addAmmo(defaultWeapon(), UNLIMITED_AMMO);
  curweapons.resize(SIMUL_WEAPONS, defaultWeapon());
}

IDBUpgradeAdjust Player::adjustUpgrade(const IDBUpgrade *in_upg) const { return IDBUpgradeAdjust(in_upg, &adjustment); };
IDBGloryAdjust Player::adjustGlory(const IDBGlory *in_upg) const { return IDBGloryAdjust(in_upg, &adjustment); };
IDBBombardmentAdjust Player::adjustBombardment(const IDBBombardment *in_upg) const { return IDBBombardmentAdjust(in_upg, &adjustment); };
IDBWeaponAdjust Player::adjustWeapon(const IDBWeapon *in_upg) const { return IDBWeaponAdjust(in_upg, &adjustment); };

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) == ITEMSTATE_UNOWNED && adjustUpgrade(in_upg).cost() <= cash; }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) == ITEMSTATE_UNOWNED && adjustGlory(in_glory).cost() <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) == ITEMSTATE_UNOWNED && adjustBombardment(in_bombardment).cost() <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const { return adjustWeapon(in_weap).cost() <= cash && in_weap != defaultWeapon(); }

bool Player::canSellGlory(const IDBGlory *in_glory) const { return hasGlory(in_glory) && in_glory != defaultGlory(); };
bool Player::canSellBombardment(const IDBBombardment *in_bombardment) const { return hasBombardment(in_bombardment) && in_bombardment != defaultBombardment(); };
bool Player::canSellWeapon(const IDBWeapon *in_weap) const { return ammoCount(in_weap) > 0; }

void Player::buyUpgrade(const IDBUpgrade *in_upg) {
  CHECK(cash >= adjustUpgrade(in_upg).cost());
  CHECK(canBuyUpgrade(in_upg));
  cash -= adjustUpgrade(in_upg).cost();
  upgrades.push_back(in_upg);
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
  weapons.addAmmo(in_weap, in_weap->quantity);
  cash -= adjustWeapon(in_weap).cost();
}

void Player::equipGlory(const IDBGlory *in_glory) {
  CHECK(count(glory.begin(), glory.end(), in_glory));
  swap(*find(glory.begin(), glory.end(), in_glory), glory[0]);
}
void Player::equipBombardment(const IDBBombardment *in_bombardment) {
  CHECK(count(bombardment.begin(), bombardment.end(), in_bombardment));
  swap(*find(bombardment.begin(), bombardment.end(), in_bombardment), bombardment[0]);
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

int Player::hasUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) != ITEMSTATE_UNOWNED; }
int Player::hasGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) != ITEMSTATE_UNOWNED; }
int Player::hasBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) != ITEMSTATE_UNOWNED; }

int Player::stateUpgrade(const IDBUpgrade *in_upg) const {
  CHECK(in_upg);
  return count(upgrades.begin(), upgrades.end(), in_upg) * ITEMSTATE_EQUIPPED; // can't be bought without being equipped, heh
}
int Player::stateGlory(const IDBGlory *in_glory) const {
  CHECK(glory.size() >= 1);
  if(glory[0] == in_glory)
    return ITEMSTATE_EQUIPPED;
  return count(glory.begin(), glory.end(), in_glory) * ITEMSTATE_BOUGHT;
}
int Player::stateBombardment(const IDBBombardment *in_bombardment) const {
  CHECK(bombardment.size() >= 1);
  if(bombardment[0] == in_bombardment)
    return ITEMSTATE_EQUIPPED;
  return count(bombardment.begin(), bombardment.end(), in_bombardment) * ITEMSTATE_BOUGHT;
}

const IDBFaction *Player::getFaction() const {
  return faction; };

IDBGloryAdjust Player::getGlory() const {
  return IDBGloryAdjust(glory[0], &adjustment); };
IDBBombardmentAdjust Player::getBombardment() const {
  return IDBBombardmentAdjust(bombardment[0], &adjustment); };
IDBTankAdjust Player::getTank() const {
  return IDBTankAdjust(NULL, &adjustment); };

IDBWeaponAdjust Player::getWeapon(int id) const {
  return IDBWeaponAdjust(weapons.getWeaponSlot(id), &adjustment); };
void Player::cycleWeapon(int id) {
  weapons.cycleWeapon(id);
}

Money Player::getCash() const {
  return cash;
}
void Player::addCash(Money amount) {
  CHECK(amount.toFloat() >= 0);
  dprintf("Adding %s bucks!\n", amount.textual().c_str());
  cash += amount;
}

void Player::addKill() { kills++; }
void Player::addWin() { wins++; }
void Player::addDamage(float damage) { damageDone += damage; }

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
bool Player::getWeaponEquipBit(const IDBWeapon *weapon, int id) const {
  return weapons.getWeaponEquipBit(weapon, id);
}

Player::Player() {
  cash = Money(-1);
  faction = NULL;
}

Player::Player(const IDBFaction *fact, int in_factionmode) {
  faction = fact;
  factionmode = in_factionmode;
  CHECK(factionmode >= 0 && factionmode < FACTIONMODE_LAST);
  cash = Money(FLAGS_startingCash);
  reCalculate();
  glory.push_back(defaultGlory());
  bombardment.push_back(defaultBombardment());
}

void Player::reCalculate() {
  //adjustment = *faction->adjustment[factionmode];
  adjustment = *faction->adjustment[0]; // because factions aren't really useful yet
  for(int i = 0; i < upgrades.size(); i++)
    adjustment += *upgrades[i]->adjustment;
  adjustment.debugDump();
}
