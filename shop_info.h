#ifndef DNET_SHOPINFO
#define DNET_SHOPINFO

#include "player.h"

class ShopInfo {
public:
  ShopInfo();

  void init(const IDBWeapon *weap, const Player *player);
  
  void runTick();
  void renderFrame() const;

private:
  ShopInfo(const ShopInfo &rhs);
  void operator=(const ShopInfo &rhs);
};

#endif
