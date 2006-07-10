#ifndef DNET_GAME_AI
#define DNET_GAME_AI

#include "input.h"
#include "rng.h"

using namespace std;

class Tank;
class Coord2;

class GameAi {
protected:
  Keystates nextKeys;
  Rng rng;
  
public:
  
  virtual void updateGame(const vector<Tank> &players, int me) = 0;
  virtual void updateBombardment(const vector<Tank> &players, Coord2 mypos) = 0;

  Keystates getNextKeys() const;

  GameAi();
  virtual ~GameAi();

protected:
  
  void zeroNextKeys();
  void normalizeNext();

private:
  
  // do not implement
  GameAi(const GameAi &foo);
  GameAi &operator=(const GameAi &foo);

};

#endif
