#ifndef DNET_SHOPDEMO
#define DNET_SHOPDEMO

#include "player.h"
#include "game.h"

class ShopDemo {
public:
  ShopDemo();

  void init(const IDBWeapon *weap, const Player *player);
  
  void runTick();
  void renderFrame() const;

private:

  Game game;
  vector<Player> players;

  ShopDemo(const ShopDemo &rhs);
  void operator=(const ShopDemo &rhs);
};

#endif
