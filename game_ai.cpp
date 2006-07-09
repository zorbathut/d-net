
#include "game_ai.h"

#include "game.h"

void GameAi::updateGame(const vector<Tank> &players, int me) {
  zeroNextKeys();
  if(rng.frand() < 0.01) {
    // find a tank, because approach and retreat both need one
    int targtank;
    {
      vector<int> validtargets;
      for(int i = 0; i < players.size(); i++)
        if(i != me && players[i].live)
          validtargets.push_back(i);
      if(validtargets.size() == 0)
        validtargets.push_back(me);
      targtank = validtargets[int(validtargets.size() * rng.frand())];
    }
    
    float neai = rng.frand();
    if(neai < 0.6) {
      gamemode = AGM_APPROACH;
      targetplayer = targtank;
    } else if(neai < 0.7) {
      gamemode = AGM_RETREAT;
      targetplayer = targtank;
    } else if(neai < 0.9) {
      gamemode = AGM_WANDER;
      targetdir.x = rng.frand() - 0.5;
      targetdir.y = rng.frand() - 0.5;
      targetdir = normalize(targetdir);
    } else {
      gamemode = AGM_BACKUP;
    }
  }
  Float2 mypos = players[me].pos.toFloat();
  if(gamemode == AGM_APPROACH) {
    Float2 enepos = players[targetplayer].pos.toFloat();
    enepos -= mypos;
    enepos.y *= -1;
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.udlrax = enepos;
  } else if(gamemode == AGM_RETREAT) {
    Float2 enepos = players[targetplayer].pos.toFloat();
    enepos -= mypos;
    enepos.y *= -1;
    enepos *= -1;
    
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.udlrax = enepos;
  } else if(gamemode == AGM_WANDER) {
    nextKeys.udlrax = targetdir;
  } else if(gamemode == AGM_BACKUP) {
    Float2 nx(-makeAngle(players[me].d));
    nx.x += (rng.frand() - 0.5) / 100;
    nx.y += (rng.frand() - 0.5) / 100;
    nx = normalize(nx);
    nextKeys.udlrax = nx;
  }
  for(int i = 0; i < SIMUL_WEAPONS; i++) {
    if(rng.frand() < 0.001)
      firing[i] = !firing[i];
    nextKeys.fire[i].down = firing;
    nextKeys.change[i].down = (rng.frand() < 0.001);  // weapon switch
  }
  
  normalizeNext();
}

void GameAi::updateBombardment(const vector<Tank> &players, Coord2 mypos) {
  zeroNextKeys();
  Coord2 clopos(0, 0);
  Coord clodist = 1000000;
  for(int i = 0; i < players.size(); i++) {
    if(players[i].live && len(players[i].pos - mypos) < clodist) {
      clodist = len(players[i].pos - mypos);
      clopos = players[i].pos;
    }
  }
  Coord2 dir = clopos - mypos;
  if(len(dir) != 0)
    dir = normalize(dir);
  nextKeys.udlrax = dir.toFloat();
  nextKeys.udlrax.y *= -1;
  if(clodist < 10)
    nextKeys.fire[0].down = (rng.frand() < 0.02);
}

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
  memset(firing, 0, sizeof(firing));
}
