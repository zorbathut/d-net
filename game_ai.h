#ifndef DNET_GAME_AI
#define DNET_GAME_AI

#include "input.h"
#include "rng.h"

using namespace std;

class Tank;
class Coord2;

class GameAi {
public:
  
  void updateGame(const vector<Tank> &players, int me);
  void updateBombardment(const vector<Tank> &players, Coord2 mypos);

  Keystates getNextKeys() const;

  GameAi();
  virtual ~GameAi();

protected:

  Keystates nextKeys;
  Rng rng;

  void zeroNextKeys();
  void normalizeNext();

private:
  
  virtual void updateGameWork(const vector<Tank> &players, int me) = 0;
  virtual void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) = 0;
  
  // do not implement
  GameAi(const GameAi &foo);
  GameAi &operator=(const GameAi &foo);

};

#endif
