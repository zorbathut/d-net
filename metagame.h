#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "game.h"
#include "metagame_config.h"

using namespace std;

class Ai;

class Shop {
private:
  Player *player;

  vector<int> curloc;

  float xofs;
  vector<float> expandy;

  bool selling;
  bool disabled;  // when we switch, we wait for them to let go of the button

  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCurNode() const;
  const HierarchyNode &getCategoryNode() const;

  void doTableUpdate();
  void doTableRender() const;
  void renderNode(const HierarchyNode &node, int depth) const;

public:
  bool runTick( const Keystates &keys );
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();
  Shop(Player *player);
};

class Metagame {
  
  int mode;
  int gameround;
  
  int faction_mode;
  
  enum { MGM_PLAYERCHOOSE, MGM_FACTIONTYPE, MGM_SHOP, MGM_PLAY };
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
  bool runTick( const vector< Controller > &keys );

private:
  
  void calculateLrStats();
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;

  void findLevels(int playercount);

  void operator=(const Metagame &rhs);
  Metagame(const Metagame &rhs);  // do not implement, fuckers
};

#endif
