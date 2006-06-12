
#include "player.h"

#include "args.h"
#include "const.h"

using namespace std;

DEFINE_int(startingCash, 1000, "Cash to start with");

IDBUpgradeAdjust Player::adjustUpgrade(const IDBUpgrade *in_upg) const { return IDBUpgradeAdjust(in_upg, &adjustment); };
IDBGloryAdjust Player::adjustGlory(const IDBGlory *in_upg) const { return IDBGloryAdjust(in_upg, &adjustment); };
IDBBombardmentAdjust Player::adjustBombardment(const IDBBombardment *in_upg) const { return IDBBombardmentAdjust(in_upg, &adjustment); };
IDBWeaponAdjust Player::adjustWeapon(const IDBWeapon *in_upg) const { return IDBWeaponAdjust(in_upg, &adjustment); };

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return stateUpgrade(in_upg) == ITEMSTATE_UNOWNED && adjustUpgrade(in_upg).cost() <= cash; }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return stateGlory(in_glory) == ITEMSTATE_UNOWNED && adjustGlory(in_glory).cost() <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return stateBombardment(in_bombardment) == ITEMSTATE_UNOWNED && adjustBombardment(in_bombardment).cost() <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const { return adjustWeapon(in_weap).cost() <= cash; }

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
  if(weapons[make_pair(in_weap->name, in_weap)] != -1)
    weapons[make_pair(in_weap->name, in_weap)] += in_weap->quantity;
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
  CHECK(weapons.count(make_pair(in_weap->name, in_weap)));
  CHECK(weapons[make_pair(in_weap->name, in_weap)] > 0);

  int sold = min(ammoCount(in_weap), in_weap->quantity);
  cash += adjustWeapon(in_weap).sellcost(sold);
  consumeAmmo(in_weap, sold);
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
int Player::ammoCount(const IDBWeapon *in_weapon) const {
  if(!weapons.count(make_pair(in_weapon->name, in_weapon)))
    return 0;
  return weapons.find(make_pair(in_weapon->name, in_weapon))->second;
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
  return IDBWeaponAdjust(curweapons[id], &adjustment); };
void Player::cycleWeapon(int id) {
  map<pair<string, const IDBWeapon *>, int>::iterator itr = weapons.find(make_pair(curweapons[id]->name, curweapons[id]));
  CHECK(itr != weapons.end());
  itr++;
  if(itr == weapons.end())
    curweapons[id] = weapons.begin()->first.second;
  else
    curweapons[id] = itr->first.second;
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
  CHECK(weapons.count(make_pair(curweapons[id]->name, curweapons[id])));
  float cost = adjustWeapon(curweapons[id]).cost().toFloat() / curweapons[id]->quantity;
  if(ammoCount(curweapons[id]) != -1)
    consumeAmmo(curweapons[id], 1);
  return cost;
};

int Player::shotsLeft(int id) const {
  CHECK(weapons.count(make_pair(curweapons[id]->name, curweapons[id])));
  return weapons.find(make_pair(curweapons[id]->name, curweapons[id]))->second;
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
  weapons[make_pair(defaultWeapon()->name, defaultWeapon())] = -1;
  curweapons.resize(SIMUL_WEAPONS, defaultWeapon());
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

void Player::consumeAmmo(const IDBWeapon *weapon, int count) {
  int *ammo = &weapons[make_pair(weapon->name, weapon)];
  CHECK(*ammo >= count);
  (*ammo) -= count;
  if(*ammo == 0) {
    for(int i = 0; i < curweapons.size(); i++) {
      if(curweapons[i] == weapon) {
        cycleWeapon(i);
        CHECK(curweapons[i] != weapon); // fix this
      }
    }
    weapons.erase(make_pair(weapon->name, weapon));
  }
}
