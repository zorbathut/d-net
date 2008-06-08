#ifndef DNET_SHOPINFO
#define DNET_SHOPINFO

#include "shop_demo.h"
using namespace std;


class ShopInfo {
public:
  ShopInfo();

  void init(const IDBWeapon *in_weapon, const Player *player, int playercount, bool equip_info, bool miniature);
  void init(const IDBGlory *in_glory, const Player *player, bool miniature);
  void init(const IDBBombardment *in_bombardment, const Player *player, bool miniature);
  void init(const IDBUpgrade *in_upgrade, const Player *player, bool miniature);
  void init(const IDBTank *in_tank, const Player *player, bool miniature);
  void init(const IDBImplant *in_implant, bool upgrade, const Player *player, bool miniature);
  void init(const IDBImplantSlot *in_implant, const Player *player, bool miniature);

  void initIfNeeded(const IDBWeapon *in_weapon, const Player *player, int playercount, bool equip_info, bool miniature);
  void initIfNeeded(const IDBGlory *in_glory, const Player *player, bool miniature);
  void initIfNeeded(const IDBBombardment *in_bombardment, const Player *player, bool miniature);
  void initIfNeeded(const IDBUpgrade *in_upgrade, const Player *player, bool miniature);
  void initIfNeeded(const IDBTank *in_tank, const Player *player, bool miniature);
  void initIfNeeded(const IDBImplant *in_implant, bool upgrade, const Player *player, bool miniature);
  void initIfNeeded(const IDBImplantSlot *in_implant, const Player *player, bool miniature);

  void clear();
  
  void runTick();
  void renderFrame(Float4 bounds, float fontsize, Float4 inset, const Player *player) const;

private:
  
  void null();

  static string formatChange(IDBAdjustment::IDBAType cat, const Player &before, const Player &after, const IDBAdjustment &adjust);
  static string formatSlot(IDBAdjustment::IDBAType cat, const Player &player);

  Player getUnupgradedPlayer(const Player *player) const;
  Player getUpgradedPlayer(const Player *player) const;

  Player getUnimplantedPlayer(const Player *player) const;
  Player getImplantedPlayer(const Player *player) const;
  Player getImplantedLeveledPlayer(const Player *player) const;

  bool hasDemo() const;

  ShopDemo demo;
  bool miniature;

  const IDBWeapon *weapon;
  bool weapon_equipinfo;
  const IDBGlory *glory;
  const IDBBombardment *bombardment;
  const IDBUpgrade *upgrade;
  const IDBTank *tank;
  const IDBImplant *implant;
  bool implant_upgrade;
  const IDBImplantSlot *implantslot;

  const string *text;

  int playercount;

  ShopInfo(const ShopInfo &rhs);
  void operator=(const ShopInfo &rhs);
};

#endif
