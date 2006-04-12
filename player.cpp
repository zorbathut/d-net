
#include "player.h"

#include "args.h"

DEFINE_int(startingCash, 1000, "Cash to start with");

float Player::maxHealth() const {
  return max_health;
}

float Player::turnSpeed() const {
  return turn_speed;
}

Coord Player::maxSpeed() const {
  return max_speed;
}

Money Player::costUpgrade(const IDBUpgrade *in_upg) const { return in_upg->base_cost; };
Money Player::costGlory(const IDBGlory *in_glory) const { return in_glory->base_cost; };
Money Player::costBombardment(const IDBBombardment *in_bombardment) const { return in_bombardment->base_cost; };
Money Player::costWeapon(const IDBWeapon *in_weap) const { return in_weap->base_cost; };

bool Player::canBuyUpgrade(const IDBUpgrade *in_upg) const { return !hasUpgrade(in_upg) && costUpgrade(in_upg) <= cash; }; 
bool Player::canBuyGlory(const IDBGlory *in_glory) const { return !hasGlory(in_glory) && costGlory(in_glory) <= cash; };
bool Player::canBuyBombardment(const IDBBombardment *in_bombardment) const { return !hasBombardment(in_bombardment) && costBombardment(in_bombardment) <= cash; };
bool Player::canBuyWeapon(const IDBWeapon *in_weap) const {
  if(weapon == in_weap) {
    return costWeapon(in_weap) <= cash;
  } else {
    return costWeapon(in_weap) <= cash + resellAmmoValue();
  }
}

void Player::buyUpgrade(const IDBUpgrade *in_upg) {
  CHECK(cash >= costUpgrade(in_upg));
  CHECK(canBuyUpgrade(in_upg));
  cash -= costUpgrade(in_upg);
  upgrades.push_back(in_upg);
  reCalculate();
}
void Player::buyGlory(const IDBGlory *in_glory) {
  CHECK(cash >= costGlory(in_glory));
  CHECK(canBuyGlory(in_glory));
  cash -= costGlory(in_glory);
  glory = in_glory;
}
void Player::buyBombardment(const IDBBombardment *in_bombardment) {
  CHECK(cash >= costBombardment(in_bombardment));
  CHECK(canBuyBombardment(in_bombardment));
  cash -= costBombardment(in_bombardment);
  bombardment = in_bombardment;
}
void Player::buyWeapon(const IDBWeapon *in_weap) {
  CHECK(canBuyWeapon(in_weap));
  if(weapon == in_weap) {
    if(shots_left != -1)
      shots_left += in_weap->quantity;
  } else {
    cash += resellAmmoValue();
    weapon = in_weap;
    shots_left = in_weap->quantity;
    if(weapon == defaultWeapon())
      shots_left = -1;
  }
  cash -= costWeapon(in_weap);
}

bool Player::hasUpgrade(const IDBUpgrade *in_upg) const {
  CHECK(in_upg);
  return count(upgrades.begin(), upgrades.end(), in_upg);
}
bool Player::hasGlory(const IDBGlory *in_glory) const { return glory == in_glory; };
bool Player::hasBombardment(const IDBBombardment *in_bombardment) const { return bombardment == in_bombardment; };
bool Player::hasWeapon(const IDBWeapon *in_weap) const { return weapon == in_weap; };

const IDBFaction *Player::getFaction() const { return faction; };
const IDBGlory *Player::getGlory() const { return glory; };
const IDBBombardment *Player::getBombardment() const { return bombardment; }
const IDBWeapon *Player::getWeapon() const { return weapon; };

Money Player::resellAmmoValue() const {
  return costWeapon(weapon) * shotsLeft() * 10 / (8 * weapon->quantity);
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

float Player::shotFired() {
  float cost = costWeapon(weapon).toFloat() / weapon->quantity;
  if(shots_left != -1)
    shots_left--;
  if(shots_left == 0) {
    weapon = defaultWeapon();
    shots_left = -1;
  }
  return cost;
};

int Player::shotsLeft() const {
  return shots_left;
}

Player::Player() {
  cash = Money(-1);
  faction = NULL;
  weapon = NULL;
  glory = NULL;
  bombardment = NULL;
  shots_left = -2;
}

Player::Player(const IDBFaction *fact) {
  faction = fact;
  cash = Money(FLAGS_startingCash);
  reCalculate();
  weapon = defaultWeapon();
  glory = defaultGlory();
  bombardment = defaultBombardment();
  shots_left = -1;
}

void Player::reCalculate() {
  max_health = 20;
  turn_speed = 2.f / FPS;
  max_speed = Coord(24) / FPS;
  int healthMult = 100;
  int turnMult = 100;
  int speedMult = 100;
  for(int i = 0; i < upgrades.size(); i++) {
    healthMult += upgrades[i]->hull;
    turnMult += upgrades[i]->handling;
    speedMult += upgrades[i]->engine;
  }
  max_health *= healthMult;
  max_health /= 100;
  turn_speed *= turnMult;
  turn_speed /= 100;
  max_speed *= speedMult;
  max_speed /= 100;
}
