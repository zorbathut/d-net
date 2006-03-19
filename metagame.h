#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "util.h"
#include "game.h"
#include "gfx.h"
#include "itemdb.h"
#include "level.h"

#include <vector>
#include <string>

using namespace std;

class Ai;

class Shop {
private:
  Player *player;

  vector<int> curloc;

  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCurNode() const;
  const HierarchyNode &getCategoryNode() const;

  void renderNode(const HierarchyNode &node, int depth) const;

public:
  bool runTick( const Keystates &keys );
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();
  Shop(Player *player);
};

struct PlayerMenuState {
public:
  int playerkey;
  int playersymbol;
  Float2 playerpos;
  int playermode;

  PlayerMenuState();
  PlayerMenuState(Float2 cent);
};

class Metagame {
  
  int mode;
  int gameround;
  
  enum { MGM_PLAYERCHOOSE, MGM_SHOP, MGM_PLAY };
  int currentShop;
  
  vector<PlayerMenuState> pms;  // heh.
  
  vector<Dvec2> symbols;
  vector<Float4> symbolpos;
  vector<int> symboltaken;
  
  vector<Level> levels;
  
  Game game;
  
  Shop shop;
  
  vector<Player> playerdata;
  
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<int> lrCash;
  vector<bool> checked;
  
  vector<int> fireHeld;
  
  int roundsBetweenShop;

public:

  void renderToScreen() const;
  void ai(const vector<Ai *> &ai) const;
  bool runTick( const vector< Controller > &keys );

  void calculateLrStats();
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;

  void findLevels(int playercount);

  Metagame();
  Metagame(int playercount, int roundsBetweenShop);

};

#endif
