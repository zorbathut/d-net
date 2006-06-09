#ifndef DNET_AI
#define DNET_AI

#include "input.h"
#include "rng.h"

#include <deque>

using namespace std;

class FactionState;
class PlayerMenuState;
class Player;
class Tank;
class Coord2;


class Ai {
private:
  enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
  deque<Controller> shopQueue;
  Controller nextKeys;
  Rng rng;
public: bool shopdone;
  
  int gamemode;
  int targetplayer;
  Float2 targetdir;
  bool firing;
  
public:
  
  void updatePregame();
  void updateCharacterChoice(const vector<FactionState> &factions, const vector<PlayerMenuState> &players, int me);
  void updateShop(const Player *player);
  void updateGame(const vector<Tank> &players, int me);
  void updateBombardment(const vector<Tank> &players, Coord2 mypos);
  void updateWaitingForReport();

  Controller getNextKeys() const;

  Ai();

private:
  
  void zeroNextKeys();

};

#endif
