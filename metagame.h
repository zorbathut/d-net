#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "game.h"
#include "metagame_tween.h"

using namespace std;

class Ai;

class Metagame {
  
  enum { MGM_PLAYERCHOOSE, MGM_FACTIONTYPE, MGM_TWEEN, MGM_PLAY };
  int mode;
  
  int faction_mode;
  vector<Player> faction_mode_players;
  // these are the players we use for faction_mode so we don't modify existing players
  
  PersistentData persistent;
  
  vector<Level> levels;
  
  Game game;
  vector<const IDBFaction *> win_history;
  
  int gameround;
  int roundsBetweenShop;

public:

  Metagame(int playercount, int roundsBetweenShop);

  void renderToScreen() const;
  void ai(const vector<Ai *> &ai) const;
  bool runTick(const vector< Controller > &keys);

private:

  void findLevels(int playercount);

  void operator=(const Metagame &rhs);
  Metagame(const Metagame &rhs);  // do not implement, fuckers
};

#endif
