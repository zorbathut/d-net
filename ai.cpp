
#include "ai.h"

#include "itemdb.h"

void Ai::updatePregame() {
  zeroNextKeys();
  nextKeys.menu = Float2(0, 0);
  nextKeys.keys[0].down = true;
}

void Ai::updateCharacterChoice(const vector<FactionState> &factions, const vector<PlayerMenuState> &players, int you) {
  zeroNextKeys();
  if(!players[you].faction) {
    int targfact = you;
    if(targfact >= factions.size() || targfact < 0)
      targfact = -1;
    if(targfact == -1) {
      nextKeys.menu = Float2(0, 0);
      nextKeys.keys[0].down = true;
      return;
    }
    Float2 targpt = factions[targfact].compass_location.midpoint() - players[you].compasspos;
    if(len(targpt) != 0)
      targpt = normalize(targpt);
    nextKeys.menu = Float2(targpt.x, -targpt.y);
    nextKeys.keys[0].down = isInside(factions[targfact].compass_location, players[you].compasspos);
  } else if(players[you].settingmode == SETTING_BUTTONS) {
    if(players[you].setting_button_current >= 0 && players[you].setting_button_current < nextKeys.keys.size())
      nextKeys.keys[players[you].setting_button_current].down = frameNumber % 2;
  } else if(players[you].settingmode == SETTING_AXISTYPE) {
    if(players[you].setting_axistype != KSAX_ABSOLUTE)
      nextKeys.menu = Float2(1.0, 0);
    else
      nextKeys.keys[BUTTON_ACCEPT].down = true;
  } else if(players[you].settingmode == SETTING_READY) {
    nextKeys.keys[BUTTON_ACCEPT].down = true;
  } else {
    CHECK(0);
  }
}

Controller makeController(float x, float y, bool key) {
  Controller rv;
  rv.keys.resize(BUTTON_LAST);
  rv.menu = Float2(x, y);
  rv.keys[0].down = key;
  for(int i = 1; i < BUTTON_LAST; i++)
    rv.keys[i].down = key;
  return rv;
}

void doMegaEnumWorker(const HierarchyNode &rt, vector<pair<int, vector<Controller> > > *weps, vector<pair<pair<int, const IDBUpgrade*>, vector<Controller> > > *upgs, vector<Controller> *done, vector<Controller> path) {
  if(rt.type == HierarchyNode::HNT_CATEGORY) {
    path.push_back(makeController(1, 0, 0));
    for(int i = 0; i < rt.branches.size(); i++) {
      if(i)
        path.push_back(makeController(0, -1, 0));
      doMegaEnumWorker(rt.branches[i], weps, upgs, done, path);
    }
  } else if(rt.type == HierarchyNode::HNT_WEAPON) {
    weps->push_back(make_pair(rt.cost, path));
  } else if(rt.type == HierarchyNode::HNT_UPGRADE) {  // TODO: don't buy stuff if you already have it :)
    upgs->push_back(make_pair(make_pair(rt.cost, rt.upgrade), path));
  } else if(rt.type == HierarchyNode::HNT_GLORY) {
    upgs->push_back(make_pair(make_pair(rt.cost, (IDBUpgrade*)NULL), path));
  } else if(rt.type == HierarchyNode::HNT_BOMBARDMENT) {
    upgs->push_back(make_pair(make_pair(rt.cost, (IDBUpgrade*)NULL), path));
  } else if(rt.type == HierarchyNode::HNT_DONE) {
    CHECK(done->size() == 0);
    *done = path;
  } else {
    CHECK(0);
  }
}

void doMegaEnum(const HierarchyNode &rt, vector<pair<int, vector<Controller> > > *weps, vector<pair<pair<int, const IDBUpgrade*>, vector<Controller> > > *upgs, vector<Controller> *done) {
  for(int i = 0; i < rt.branches.size(); i++) {
    vector<Controller> tvd;
    for(int j = 0; j < i; j++)
      tvd.push_back(makeController(0, -1, 0));
    doMegaEnumWorker(rt.branches[i], weps, upgs, done, tvd);
  }
}

vector<Controller> reversecontroller(const vector<Controller> &in) {
  vector<Controller> rv;
  for(int i = 0; i < in.size(); i++)
    rv.push_back(makeController(-in[i].menu.x, -in[i].menu.y, in[i].keys[0].down));
  reverse(rv.begin(), rv.end());
  return rv;
}

void appendPurchases(deque<Controller> *dest, const vector<Controller> &src, int count) {
  dest->insert(dest->end(), src.begin(), src.end());
  for(int i = 0; i < count; i++)
    dest->push_back(makeController(0, 0, 1));
  vector<Controller> reversed = reversecontroller(src);
  dest->insert(dest->end(), reversed.begin(), reversed.end());
}

void Ai::updateShop(const Player *player) {
  zeroNextKeys();
  if(shopQueue.size()) {
    nextKeys = shopQueue[0];
    shopQueue.pop_front();
    return;
  }
  CHECK(!shopdone);
  const HierarchyNode &rt = itemDbRoot();
  vector<pair<int, vector<Controller> > > weps;
  vector<pair<pair<int, const IDBUpgrade *>, vector<Controller> > > upgs;
  vector<Controller> done;
  doMegaEnum(rt, &weps, &upgs, &done);
  dprintf("%d weps, %d upgs, %d donesize\n", weps.size(), upgs.size(), done.size());
  CHECK(weps.size());
  CHECK(done.size());
  sort(weps.begin(), weps.end());
  sort(upgs.begin(), upgs.end());
  int upgcash = player->cash / 2;
  int weapcash = player->cash;
  while(upgcash) {
    for(int i = 0; i < upgs.size(); i++) {
      if(upgs[i].first.second && player->hasUpgrade(upgs[i].first.second)) {
        upgs.erase(upgs.begin() + i);
        i--;
      }
    }
    int dlim = 0;
    while(dlim < upgs.size() && upgs[dlim].first.first <= upgcash)
      dlim++;
    if(dlim == 0)
      break;
    dlim = int(dlim * rng.frand());
    upgcash -= upgs[dlim].first.first;
    weapcash -= upgs[dlim].first.first;
    appendPurchases(&shopQueue, upgs[dlim].second, 1);
    upgs.erase(upgs.begin() + dlim);
  }
  if(weapcash) {
    int dlim = 0;
    while(dlim < weps.size() && weps[dlim].first <= weapcash)
      dlim++;
    dlim = int(dlim * rng.frand());
    int amount = 1;
    if(weps[dlim].first) {
      amount = min(weapcash / weps[dlim].first, 100);
    }
    appendPurchases(&shopQueue, weps[dlim].second, amount);
  }
  shopQueue.insert(shopQueue.end(), done.begin(), done.end());
  shopQueue.push_back(makeController(0, 0, 1));
  //for(int i = 0; i < shopQueue.size(); i++)
    //dprintf("%f %f %d\n", shopQueue[i].x, shopQueue[i].y, shopQueue[i].keys[0].down);
  deque<Controller> realShopQueue;
  for(int i = 0; i < shopQueue.size(); i++) {
    realShopQueue.push_back(shopQueue[i]);
    realShopQueue.push_back(makeController(0, 0, 0));
  }
  swap(shopQueue, realShopQueue);
  shopdone = true;
  updateShop(player);
}

void Ai::updateGame(const vector<vector<Coord2> > &collide, const vector<Tank> &players, int me) {
  zeroNextKeys();
  if(shopdone || rng.frand() < 0.01) {
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
      targetdir.x = rng.frand();
      targetdir.y = rng.frand();
      targetdir = normalize(targetdir);
    } else {
      gamemode = AGM_BACKUP;
    }
    //dprintf("Tank %d changing plan, %d\n", me, gamemode);
    shopdone = false;
  }
  Float2 mypos = players[me].pos.toFloat();
  if(gamemode == AGM_APPROACH) {
    Float2 enepos = players[targetplayer].pos.toFloat();
    enepos -= mypos;
    enepos.y *= -1;
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.menu = enepos;
  } else if(gamemode == AGM_RETREAT) {
    Float2 enepos = players[targetplayer].pos.toFloat();
    enepos -= mypos;
    enepos.y *= -1;
    enepos *= -1;
    
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.menu = enepos;
  } else if(gamemode == AGM_WANDER) {
    nextKeys.menu = targetdir;
  } else if(gamemode == AGM_BACKUP) {
    Float2 nx(-makeAngle(players[me].d));
    nx.x += (rng.frand() - 0.5) / 100;
    nx.y += (rng.frand() - 0.5) / 100;
    nx = normalize(nx);
    nextKeys.menu = nx;
  }
  if(rng.frand() < 0.001)
    firing = !firing;
  nextKeys.keys[0].down = firing;
}

void Ai::updateBombardment(const vector<Tank> &players, Coord2 mypos) {
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
  nextKeys.menu = dir.toFloat();
  nextKeys.menu.y *= -1;
  nextKeys.keys[0].down = false;
  if(clodist < 10)
    nextKeys.keys[0].down = (rng.frand() < 0.02);
}

void Ai::updateWaitingForReport() {
  zeroNextKeys();
  nextKeys.menu = Float2(0, 0);
  nextKeys.keys[0].down = true;
}

Controller Ai::getNextKeys() const {
  return nextKeys;
}

Ai::Ai() {
  nextKeys.keys.resize(BUTTON_LAST);
  shopdone = false;
  firing = false;
}

void Ai::zeroNextKeys() {
  nextKeys.menu = Float2(0, 0);
  for(int i = 0; i < nextKeys.keys.size(); i++)
    nextKeys.keys[i].down = false;
}

