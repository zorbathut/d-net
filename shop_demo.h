#ifndef DNET_SHOPDEMO
#define DNET_SHOPDEMO

#include "game.h"

using namespace std;

class GameAiScatterbombing;
class GameAiKamikaze;
class GameAiMining;
class GameAiTraversing;

class ShopDemo {
public:
  void init(const IDBWeapon *weap, const Player *player, Recorder *recorder);
  void init(const IDBBombardment *weap, const Player *player, Recorder *recorder);
  void init(const IDBGlory *weap, const Player *player, Recorder *recorder);
  
  void runSingleTick();
  void runTick();
  void renderFrame() const;

  vector<float> getStats() const;

  void dumpMetastats(Recorder *recorder) const;

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

  bool mine_mined;
  vector<GameAiMining *> mine_miners;
  GameAiTraversing *mine_traverser;

  // don't use
  ShopDemo(const ShopDemo &rhs);
  void operator=(const ShopDemo &rhs);
};

#endif
