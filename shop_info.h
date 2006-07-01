#ifndef DNET_SHOPINFO
#define DNET_SHOPINFO

#include "shop_demo.h"

class ShopInfo {
public:
  ShopInfo();

  void init(const IDBWeapon *weap, const Player *player);
  
  void runTick();
  void renderFrame(Float4 bounds, float fontsize, Float4 inset) const;

private:

  ShopDemo demo;

  const IDBWeapon *weapon;
  const Player *player;

  ShopInfo(const ShopInfo &rhs);
  void operator=(const ShopInfo &rhs);
};

#endif
