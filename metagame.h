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
  bool instant_action_keys;

  Rng rng;

public:

  Metagame(const vector<bool> &humans, Money startingcash, Coord multiple, int faction, int roundsBetweenShop, int rounds_until_end, RngSeed seed, const InputSnag &isnag);
  void instant_action_init(const ControlConsts &ck, int primary_id);

  void renderToScreen(const InputSnag &is) const;
  void ai(const vector<Ai *> &ai, const vector<bool> &humans) const;
  bool isWaitingOnAi() const;
  bool runTick(const vector<Controller> &keys, bool confused, const InputSnag &is);

  void endgame();

  void checksum(Adler32 *adl) const;

private:

  void findLevels(int playercount);
  Level chooseLevel();

};

#endif
