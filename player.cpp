
#include "player.h"

#include "args.h"

DEFINE_int(startingCash, 1000, "Cash to start with");

IDBUpgradeAdjust Player::adjustUpgrade(const IDBUpgrade *in_upg) const { return IDBUpgradeAdjust(in_upg, &adjustment); };
IDBGloryAdjust Player::adjustGlory(const IDBGlory *in_upg) const { return IDBGloryAdjust(in_upg, &adjustment); };
IDBBombardmentAdjust Player::adjustBombardment(const IDBBombardment *in_upg) const { return IDBBombardmentAdjust(in_upg, &adjustment); };
IDBWeaponAdjust Player::adjustWeapon(const IDBWeapon *in_upg) const { return IDBWeaponAdjust(in_upg, &adjustment); };

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return !hasUpgrade(in_upg) && adjustUpgrade(in_upg).cost() <= cash; }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return !hasGlory(in_glory) && adjustGlory(in_glory).cost() <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return !hasBombardment(in_bombardment) && adjustBombardment(in_bombardment).cost() <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const {
  return adjustWeapon(in_weap).cost() <= cash;
}

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
  glory = in_glory;
}
void Player::buyBombardment(const IDBBombardment *in_bombardment) {
  CHECK(cash >= adjustBombardment(in_bombardment).cost());
  CHECK(canBuyBombardment(in_bombardment));
  cash -= adjustBombardment(in_bombardment).cost() ;
  bombardment = in_bombardment;
}
void Player::buyWeapon(const IDBWeapon *in_weap) {
  CHECK(canBuyWeapon(in_weap));
  if(weapons[make_pair(in_weap->name, in_weap)] != -1)
    weapons[make_pair(in_weap->name, in_weap)] += in_weap->quantity;
  cash -= adjustWeapon(in_weap).cost();
}

bool Player::hasUpgrade(const IDBUpgrade *in_upg) const {
  CHECK(in_upg);
  return count(upgrades.begin(), upgrades.end(), in_upg);
}
bool Player::hasGlory(const IDBGlory *in_glory) const { return glory == in_glory; };
bool Player::hasBombardment(const IDBBombardment *in_bombardment) const { return bombardment == in_bombardment; };

const IDBFaction *Player::getFaction() const {
  return faction; };

IDBGloryAdjust Player::getGlory() const {
  return IDBGloryAdjust(glory, &adjustment); };
IDBBombardmentAdjust Player::getBombardment() const {
  return IDBBombardmentAdjust(bombardment, &adjustment); };
IDBTankAdjust Player::getTank() const {
  return IDBTankAdjust(NULL, &adjustment); };

IDBWeaponAdjust Player::getWeapon() const {
  return IDBWeaponAdjust(curweapon, &adjustment); };
void Player::cycleWeapon() {
  map<pair<string, const IDBWeapon *>, int>::iterator itr = weapons.find(make_pair(curweapon->name, curweapon));
  CHECK(itr != weapons.end());
  itr++;
  if(itr == weapons.end())
    curweapon = weapons.begin()->first.second;
  else
    curweapon = itr->first.second;
}

  /*
Money Player::resellAmmoValue() const {
  float shares = 1 * adjustment.adjustmentfactor(IDBAdjustment::RECYCLE_BONUS);
  float ratio = shares / (shares + 0.2);
  return adjustWeapon(weapon).cost() * shotsLeft() * ratio / weapon->quantity;
}*/

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

float Player::shotFired() {
  CHECK(weapons.count(make_pair(curweapon->name, curweapon)));
  float cost = adjustWeapon(curweapon).cost().toFloat() / curweapon->quantity;
  int *ammo = &weapons[make_pair(curweapon->name, curweapon)];
  if(*ammo != -1)
    (*ammo)--;
  if(*ammo == 0) {
    const IDBWeapon *oldwep = curweapon;
    cycleWeapon();
    CHECK(curweapon != oldwep);
    weapons.erase(make_pair(oldwep->name, oldwep));
  }
  return cost;
};

int Player::shotsLeft() const {
  CHECK(weapons.count(make_pair(curweapon->name, curweapon)));
  return weapons.find(make_pair(curweapon->name, curweapon))->second;
}

Player::Player() {
  cash = Money(-1);
  faction = NULL;
  glory = NULL;
  bombardment = NULL;
}

Player::Player(const IDBFaction *fact, int in_factionmode) {
  faction = fact;
  factionmode = in_factionmode;
  CHECK(factionmode >= 0 && factionmode < FACTIONMODE_LAST);
  cash = Money(FLAGS_startingCash);
  reCalculate();
  buyWeapon(defaultWeapon());
  curweapon = defaultWeapon();
  glory = defaultGlory();
  bombardment = defaultBombardment();
}

void Player::reCalculate() {
  adjustment = *faction->adjustment[factionmode];
  for(int i = 0; i < upgrades.size(); i++)
    adjustment += *upgrades[i]->adjustment;
  adjustment.debugDump();
}
