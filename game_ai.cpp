
#include "game_ai.h"

#include "game.h"

Keystates GameAi::getNextKeys() const {
  return nextKeys;
}

void GameAi::zeroNextKeys() {
  nextKeys = Keystates();
}

void GameAi::normalizeNext() {
  nextKeys.axmode = KSAX_ABSOLUTE;
  nextKeys.ax[0] = nextKeys.udlrax.x;
  nextKeys.ax[1] = nextKeys.udlrax.y;
  // AFAIK u, d, l, r aren't used, so I'm not going to bother with them ATM
}

GameAi::GameAi() {
}
