#ifndef DNET_SHOPDEMO
#define DNET_SHOPDEMO

#include "game.h"

using namespace std;

class GameAiScatterbombing;
class GameAiKamikaze;

class ShopDemo {
public:
  void init(const IDBWeapon *weap, const Player *player);
  void init(const IDBBombardment *weap, const Player *player);
  void init(const IDBGlory *weap, const Player *player);
  
  void runTick();
  void renderFrame() const;

  ShopDemo();
  ~ShopDemo();

private:

  GamePackage game;
  vector<smart_ptr<GameAi> > ais;

  const int *progression;

  int mode;

  vector<GameAiScatterbombing *> bombardment_scatterers;

  vector<GameAiKamikaze *> glory_kamikazes;
  bool respawn;
  void glory_respawnPlayers();

  // don't use
  ShopDemo(const ShopDemo &rhs);
  void operator=(const ShopDemo &rhs);
};

#endif
