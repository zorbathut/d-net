#ifndef DNET_GAME_AI
#define DNET_GAME_AI

#include "input.h"
#include "rng.h"

using namespace std;

class Tank;
class Coord2;

class GameAi {
private:
  enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
  Keystates nextKeys;
  Rng rng;
  
  int gamemode;
  int targetplayer;
  Float2 targetdir;
  bool firing[SIMUL_WEAPONS];
  
public:
  
  virtual void updateGame(const vector<Tank> &players, int me);
  virtual void updateBombardment(const vector<Tank> &players, Coord2 mypos);

  Keystates getNextKeys() const;

  GameAi();

private:
  
  void zeroNextKeys();
  void normalizeNext();

};

#endif
