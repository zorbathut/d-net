#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "game.h"
#include "metagame_config.h"
#include "shop.h"

using namespace std;

class Ai;

class Metagame {
  
  int mode;
  int gameround;
  
  int faction_mode;
  
  enum { MGM_PLAYERCHOOSE, MGM_FACTIONTYPE, MGM_TWEEN, MGM_PLAY };
  int currentShop;
  
  vector<PlayerMenuState> pms;  // heh.
  
  vector<FactionState> factions;
  
  vector<Level> levels;
  
  Game game;
  
  Shop shop;
  
  vector<Player> playerdata;
  
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<Money> lrCash;
  vector<bool> checked;
  
  vector<const IDBFaction *> win_history;
  
  int roundsBetweenShop;

public:

  Metagame(int playercount, int roundsBetweenShop);

  void renderToScreen() const;
  void ai(const vector<Ai *> &ai) const;
  bool runTick(const vector< Controller > &keys);

private:
  
  bool tweenTick(const vector< Controller > &keys); // returns true when done
  void tweenRender() const;
  
  void calculateLrStats();
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;

  void findLevels(int playercount);

  void operator=(const Metagame &rhs);
  Metagame(const Metagame &rhs);  // do not implement, fuckers
};

#endif
