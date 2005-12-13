#ifndef DNET_AI
#define DNET_AI

#include "input.h"
#include "util.h"
#include "game.h"
#include "rng.h"

#include <deque>

using namespace std;

struct PlayerMenuState {  // TODO: get rid of this
public:
  int playerkey;
  int playersymbol;
  Float2 playerpos;
  int playermode;

  PlayerMenuState();
  PlayerMenuState(Float2 cent);
};

class Ai {
private:
  enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
  deque<Controller> shopQueue;
  Controller nextKeys;
  Rng rng;
  bool shopdone;
  
  int gamemode;
  int targetplayer;
  Float2 targetdir;
  bool firing;
  
public:
  
  void updatePregame();
  void updateCharacterChoice(const vector<Float4> &factions, const vector<PlayerMenuState> &players, int me);
  void updateShop(const Player *player);
  void updateGame(const vector<vector<Coord2> > &collide, const vector<Tank> &players, int me);
  void updateBombardment(const vector<Tank> &players, Coord2 mypos);
  void updateWaitingForReport();

  Controller getNextKeys() const;

  Ai();

};

#endif
