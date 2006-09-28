#ifndef DNET_SHOPINFO
#define DNET_SHOPINFO

#include "shop_demo.h"

class ShopInfo {
public:
  ShopInfo();

  void init(const IDBWeapon *in_weapon, const Player *player, bool miniature);
  void init(const IDBGlory *in_glory, const Player *player, bool miniature);
  void init(const IDBBombardment *in_bombardment, const Player *player, bool miniature);
  void init(const IDBUpgrade *in_upgrade, const Player *player, bool miniature);
  void init(const IDBTank *in_tank, const Player *player, bool miniature);
  
  void runTick();
  void renderFrame(Float4 bounds, float fontsize, Float4 inset) const;

private:
  
  void null();

  string getUpgradeBefore(int cat) const;
  string getUpgradeAfter(int cat) const;

  Player getUnupgradedPlayer() const;
  Player getUpgradedPlayer() const;

  bool hasDemo() const;

  ShopDemo demo;
  bool miniature;

  const IDBWeapon *weapon;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBUpgrade *upgrade;
  const IDBTank *tank;

  const Player *player;

  const string *text;

  ShopInfo(const ShopInfo &rhs);
  void operator=(const ShopInfo &rhs);
};

#endif
