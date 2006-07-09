#ifndef DNET_AI
#define DNET_AI

#include "input.h"
#include "rng.h"
#include "game_ai.h"

#include <deque>

using namespace std;

class FactionState;
class PlayerMenuState;
class Player;
class Tank;
class Coord2;

class Ai {
private:
  deque<Controller> shopQueue;
  Controller nextKeys;
  Rng rng;
  GameAi gai;

  bool shopdone;

  enum { UNKNOWN, CORE, GAME };
  int source;
  int curframe;
  
public:
  
  void updatePregame();
  void updateCharacterChoice(const vector<FactionState> &factions, const vector<PlayerMenuState> &players, int me);
  void updateShop(const Player *player);
  GameAi &getGameAi();
  void updateWaitingForReport();

  Controller getNextKeys() const;

  Ai();

private:
  
  void zeroNextKeys();
  void updateKeys(int desiredsource);

};

#endif
