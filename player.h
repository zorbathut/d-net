#ifndef DNET_PLAYER
#define DNET_PLAYER

#include "itemdb.h"
#include "coord.h"

class Player {
public:
  
  float maxHealth() const;
  float turnSpeed() const;
  Coord maxSpeed() const;

  Money costUpgrade(const IDBUpgrade *in_upg) const;
  Money costGlory(const IDBGlory *in_glory) const;
  Money costBombardment(const IDBBombardment *in_bombardment) const;
  Money costWeapon(const IDBWeapon *in_weap) const;

  bool canBuyUpgrade(const IDBUpgrade *in_upg) const;
  bool canBuyGlory(const IDBGlory *in_glory) const;
  bool canBuyBombardment(const IDBBombardment *in_bombardment) const;
  bool canBuyWeapon(const IDBWeapon *in_weap) const;

  void buyUpgrade(const IDBUpgrade *in_upg);
  void buyGlory(const IDBGlory *in_glory);
  void buyBombardment(const IDBBombardment *in_bombardment);
  void buyWeapon(const IDBWeapon *in_weap);

  bool hasUpgrade(const IDBUpgrade *in_upg) const;
  bool hasGlory(const IDBGlory *in_glory) const;
  bool hasBombardment(const IDBBombardment *in_bombardment) const;
  bool hasWeapon(const IDBWeapon *in_weap) const;

  const IDBFaction *getFaction() const;
  const IDBGlory *getGlory() const;
  const IDBBombardment *getBombardment() const;
  const IDBWeapon *getWeapon() const;

  Money resellAmmoValue() const;
  
  Money getCash() const;
  void addCash(Money amount); // this is really designed *solely* for the income phase

  void addKill();
  void addWin();
  void addDamage(float damage);

  int consumeKills();
  int consumeWins();
  float consumeDamage();
  
  float shotFired(); // dump ammo shot, do any weapon changing needed, return value
  /* if(players[i].player->shotsLeft != -1)
        players[i].player->shotsLeft--;
      if(players[i].player->shotsLeft == 0) {
        players[i].player->weapon = defaultWeapon();
        players[i].player->shotsLeft = -1;
      } */
  int shotsLeft() const;

  Player();
  Player(const IDBFaction *fact);

private:
  
  void reCalculate();

  float max_health;
  float turn_speed;
  Coord max_speed;

  vector<const IDBUpgrade *> upgrades;
  const IDBWeapon *weapon;
  int shots_left;

  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBFaction *faction;

  Money cash;

  float damageDone;
  int kills;
  int wins;
};

#endif
