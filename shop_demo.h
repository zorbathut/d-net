#ifndef DNET_SHOPDEMO
#define DNET_SHOPDEMO

#include "player.h"
#include "game.h"

class ShopDemo {
public:
  void init(const IDBWeapon *weap, const Player *player);
  void init(const IDBBombardment *weap, const Player *player);
  
  void runTick();
  void renderFrame() const;

  ShopDemo();
  ~ShopDemo();

private:

  Game game;
  vector<Player> players;
  vector<smart_ptr<GameAi> > ais;

  const int *progression;

  // don't use
  ShopDemo(const ShopDemo &rhs);
  void operator=(const ShopDemo &rhs);
};

#endif
