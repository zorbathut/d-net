#ifndef DNET_AI
#define DNET_AI

#include "game_ai.h"
#include "input.h"
#include "rng.h"
#include <deque>
using namespace std;



class FactionState;
class PlayerMenuState;
class Player;
class Tank;
class Coord2;
  
class GameAiStandard : public GameAi {
private:
  enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
  
  int gamemode;
  int targetplayer;
  Float2 targetdir;
  bool firing[SIMUL_WEAPONS];
  
  virtual void updateGameWork(const vector<Tank> &players, int me);
  virtual void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos);

public:

  GameAiStandard();
};

class Ai {
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
  
public:
 
  void updateIdle();

  void updatePregame();
  void updateCharacterChoice(const vector<FactionState> &factions, const vector<PlayerMenuState> &players, int me);
  void updateTween(bool live, bool pending, Float2 playerpos, bool shopped, pair<float, float> fullshoprange, pair<float, float> quickshoprange, pair<float, float> donerange);
  void updateShop(const Player *player);
  GameAi *getGameAi();
  void updateWaitingForReport();

  Controller getNextKeys() const;

  Ai();

private:
  
  void zeroNextKeys();
  void updateKeys(int desiredsource);

};

#endif
