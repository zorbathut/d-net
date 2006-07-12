
#include "game_ai.h"

#include "game.h"

void GameAi::updateGame(const vector<Tank> &players, int me) {
  zeroNextKeys();
  updateGameWork(players, me);
  normalizeNext();
}
void GameAi::updateBombardment(const vector<Tank> &players, Coord2 mypos) {
  zeroNextKeys();
  updateBombardmentWork(players, mypos);
  normalizeNext();
}

Keystates GameAi::getNextKeys() const {
  return nextKeys;
}

void GameAi::zeroNextKeys() {
  nextKeys = Keystates();
  nextKeys.axmode = KSAX_ABSOLUTE;  // we put this here so we can override it
}

void GameAi::normalizeNext() {
  nextKeys.ax[0] = nextKeys.udlrax.x;
  nextKeys.ax[1] = nextKeys.udlrax.y;
  // AFAIK u, d, l, r aren't used, so I'm not going to bother with them ATM
}

GameAi::GameAi() { }
GameAi::~GameAi() { }
