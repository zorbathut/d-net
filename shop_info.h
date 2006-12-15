#ifndef DNET_SHOPINFO
#define DNET_SHOPINFO

#include "shop_demo.h"

using namespace std;

class ShopInfo {
public:
  ShopInfo();

  void init(const IDBWeapon *in_weapon, const Player *player, bool miniature);
  void init(const IDBGlory *in_glory, const Player *player, bool miniature);
  void init(const IDBBombardment *in_bombardment, const Player *player, bool miniature);
  void init(const IDBUpgrade *in_upgrade, const Player *player, bool miniature);
  void init(const IDBTank *in_tank, const Player *player, bool miniature);
  void init(const IDBImplant *in_tank, bool upgrade, const Player *player, bool miniature);

  void initIfNeeded(const IDBWeapon *in_weapon, const Player *player, bool miniature);
  void initIfNeeded(const IDBGlory *in_glory, const Player *player, bool miniature);
  void initIfNeeded(const IDBBombardment *in_bombardment, const Player *player, bool miniature);
  void initIfNeeded(const IDBUpgrade *in_upgrade, const Player *player, bool miniature);
  void initIfNeeded(const IDBTank *in_tank, const Player *player, bool miniature);
  void initIfNeeded(const IDBImplant *in_tank, bool upgrade, const Player *player, bool miniature);

  void clear();
  
  void runTick();
  void renderFrame(Float4 bounds, float fontsize, Float4 inset, const Player *player) const;

private:
  
  void null();

  string getUpgradeBefore(int cat, const Player *player) const;
  string getUpgradeAfter(int cat, const Player *player) const;

  Player getUnupgradedPlayer(const Player *player) const;
  Player getUpgradedPlayer(const Player *player) const;

  bool hasDemo() const;

  ShopDemo demo;
  bool miniature;

  const IDBWeapon *weapon;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBUpgrade *upgrade;
  const IDBTank *tank;
  const IDBImplant *implant;
  bool implant_upgrade;

  const string *text;

  ShopInfo(const ShopInfo &rhs);
  void operator=(const ShopInfo &rhs);
};

#endif
