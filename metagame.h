#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "metagame_tween.h"

using namespace std;

class Ai;

class Metagame : boost::noncopyable {
  
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
  
  int last_level;

  Rng rng;

public:

  Metagame(int playercount, Money startingcash, Coord multiple, int faction, int roundsBetweenShop, int rounds_until_end, RngSeed seed);

  void renderToScreen() const;
  void ai(const vector<Ai *> &ai, const vector<bool> &humans) const;
  bool isWaitingOnAi(const vector<bool> &humans) const;
  bool runTick(const vector<Controller> &keys);

  void checksum(Adler32 *adl) const;

private:

  void findLevels(int playercount);
  Level chooseLevel();

};

#endif
