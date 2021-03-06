#ifndef DNET_AI
#define DNET_AI

#include "game_ai.h"
#include "rng.h"

using namespace std;

class FactionState;
class PlayerMenuState;
class Player;
class Tank;
class Coord2;
class HierarchyNode;
  
class GameAiStandard : public GameAi {
private:
  enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
  
  int gamemode;
  int targetplayer;
  Coord2 targetdir;
  bool firing[SIMUL_WEAPONS];
  
  Rng *rng;
  
  virtual void updateGameWork(const vector<Tank> &players, int me);
  virtual void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos);

public:

  GameAiStandard(Rng *rng);
};

class Ai : boost::noncopyable {
private:
  deque<Controller> shopQueue;
  Controller nextKeys;
  Rng rng;
  GameAiStandard gai;

  bool shopdone;

  int shoptarget;

  enum { UNKNOWN, CORE, GAME };
  int source;
  int curframe;
  
  int targfact;
  
public:
 
  void updateIdle();

  void updatePregame();
  void updateSetup(int pos);
  void updateCharacterChoice(const vector<FactionState> &factions, const PlayerMenuState &player);
  void updateTween(bool live, bool pending, Coord2 playerpos, bool shopped, Coord2 joinrange, Coord2 shoprange, Coord2 donerange, Coord2 endrange);
  void updateShop(const Player *player, const HierarchyNode &hierarchy, bool athead);
  GameAi *getGameAi();
  void updateWaitingForReport(bool accepted);
  void updateGameEnd(bool accepted);

  Controller getNextKeys() const;

  Ai();

private:
  
  void zeroNextKeys();
  void updateKeys(int desiredsource);

};

#endif
