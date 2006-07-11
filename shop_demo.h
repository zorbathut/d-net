#ifndef DNET_SHOPDEMO
#define DNET_SHOPDEMO

#include "player.h"
#include "game.h"

class GameAiScatterbombing;

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

  int mode;

  vector<GameAiScatterbombing *> bombardment_scatterers;

  // don't use
  ShopDemo(const ShopDemo &rhs);
  void operator=(const ShopDemo &rhs);
};

#endif
