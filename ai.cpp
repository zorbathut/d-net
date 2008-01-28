
#include "ai.h"

#include "game_tank.h"
#include "metagame_config.h"
#include "player.h"

#include <list>

using namespace std;

void GameAiStandard::updateGameWork(const vector<Tank> &players, int me) {
  if(rng->frand() < 0.01 || targetplayer == -1) {
    // find a tank, because approach and retreat both need one
    int targtank;
    {
      vector<int> validtargets;
      for(int i = 0; i < players.size(); i++)
        if(i != me && players[i].isLive())
          validtargets.push_back(i);
      if(validtargets.size() == 0)
        validtargets.push_back(me);
      targtank = validtargets[int(validtargets.size() * rng->frand())];
    }
    
    float neai = rng->frand();
    if(neai < 0.6) {
      gamemode = AGM_APPROACH;
      targetplayer = targtank;
    } else if(neai < 0.7) {
      gamemode = AGM_RETREAT;
      targetplayer = targtank;
    } else if(neai < 0.9) {
      gamemode = AGM_WANDER;
      targetdir = makeAngle(rng->cfrand() * COORDPI * 2);
    } else {
      gamemode = AGM_BACKUP;
    }
  }
  Coord2 mypos = players[me].pi.pos;
  if(gamemode == AGM_APPROACH) {
    Coord2 enepos = players[targetplayer].pi.pos;
    enepos -= mypos;
    enepos.y *= -1;
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.udlrax = enepos;
  } else if(gamemode == AGM_RETREAT) {
    Coord2 enepos = players[targetplayer].pi.pos;
    enepos -= mypos;
    enepos.y *= -1;
    enepos *= -1;
    
    if(len(enepos) > 0)
      enepos = normalize(enepos);
    nextKeys.udlrax = enepos;
  } else if(gamemode == AGM_WANDER) {
    nextKeys.udlrax = targetdir;
  } else if(gamemode == AGM_BACKUP) {
    Coord2 nx(-makeAngle(players[me].pi.d));
    nx.x += (rng->frand() - 0.5) / 100;
    nx.y += (rng->frand() - 0.5) / 100;
    nx = normalize(nx);
    nextKeys.udlrax = nx;
  }
  int wepon = 0;
  for(int i = 0; i < SIMUL_WEAPONS; i++)
    if(firing[i])
      wepon++;
  
  for(int i = 0; i < SIMUL_WEAPONS; i++) {
    float chance = 0.001;
    if(firing[i]) {
      chance /= 5;  // we want to keep firing something more often than not
      chance += wepon * 0.002; // but we'd like to be firing one thing at a time ideally as well
    }
    
    if(rng->frand() < chance)
      firing[i] = !firing[i];
    nextKeys.fire[i].down = firing[i];
  }
}

void GameAiStandard::updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
  Coord2 clopos(0, 0);
  Coord clodist = 1000000;
  for(int i = 0; i < players.size(); i++) {
    if(players[i].isLive() && len(players[i].pi.pos - mypos) < clodist) {
      clodist = len(players[i].pi.pos - mypos);
      clopos = players[i].pi.pos;
    }
  }
  nextKeys.udlrax = clopos - mypos;
  nextKeys.udlrax.y *= -1;
  if(len(nextKeys.udlrax) != 0)
    nextKeys.udlrax = normalize(nextKeys.udlrax);
  if(clodist < 10)
    nextKeys.fire[0].down = (rng->frand() < 0.02);
}

GameAiStandard::GameAiStandard(Rng *rng) : rng(rng) {
  memset(firing, 0, sizeof(firing));
  targetplayer = -1;
}

// these are not meant to be meaningful
inline bool operator<(const Button &lhs, const Button &rhs) {
  if(lhs.down != rhs.down) return lhs.down < rhs.down;
  if(lhs.push != rhs.push) return lhs.push < rhs.push;
  if(lhs.release != rhs.release) return lhs.release < rhs.release;
  if(lhs.repeat != rhs.repeat) return lhs.repeat < rhs.repeat;
  if(lhs.dur != rhs.dur) return lhs.dur < rhs.dur;
  if(lhs.sincerep != rhs.sincerep) return lhs.sincerep < rhs.sincerep;
  return false;
}

inline bool operator!=(const Button &lhs, const Button &rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const Controller &lhs, const Controller &rhs) {
  if(lhs.menu != rhs.menu) return lhs.menu < rhs.menu;
  if(lhs.u != rhs.u) return lhs.u < rhs.u;
  if(lhs.d != rhs.d) return lhs.d < rhs.d;
  if(lhs.l != rhs.l) return lhs.l < rhs.l;
  if(lhs.r != rhs.r) return lhs.r < rhs.r;
  if(lhs.keys != rhs.keys) return lhs.keys < rhs.keys;
  return false;
}

void Ai::updateIdle() {
  updateKeys(CORE);
  zeroNextKeys();
}

void Ai::updatePregame() {
  updateKeys(CORE);
  
  zeroNextKeys();
  nextKeys.menu = Coord2(0, 0);
  nextKeys.keys[0].down = true;
}

void Ai::updateSetup(int pos) {
  updateKeys(CORE);
  
  zeroNextKeys();
  if(pos != 3)
    nextKeys.menu = Coord2(0, 1);
  
  if(pos == 3 && frameNumber % 2)
    nextKeys.keys[0].down = true;
}

void Ai::updateCharacterChoice(const vector<FactionState> &factions, const PlayerMenuState &player, int you) {
  updateKeys(CORE);
  
  zeroNextKeys();
  if(!player.faction) {
    int targfact = you;
    if(targfact >= factions.size() || targfact < 0)
      targfact = -1;
    if(targfact == -1) {
      nextKeys.menu = Coord2(0, 0);
      nextKeys.keys[0].down = true;
      return;
    }
    Coord2 targpt = factions[targfact].compass_location.midpoint() - player.compasspos;
    if(len(targpt) != 0)
      targpt = normalize(targpt);
    nextKeys.menu = Coord2(targpt.x, -targpt.y);
    nextKeys.keys[0].down = isInside(factions[targfact].compass_location, player.compasspos);
  } else if(player.settingmode == SETTING_BUTTONS) {
    if(player.setting_button_current >= 0 && player.setting_button_current < nextKeys.keys.size())
      nextKeys.keys[player.setting_button_current].down = frameNumber % 2;
    else if(player.setting_button_current == nextKeys.keys.size())
      nextKeys.keys[BUTTON_ACCEPT].down = frameNumber % 2;
/*  } else if(player.settingmode == SETTING_AXISCHOOSE) {
    if(player.setting_axistype != KSAX_ABSOLUTE) {
      if(frameNumber % 2 == 0) {
        nextKeys.keys[BUTTON_FIRE1].down = true;
        nextKeys.keys[BUTTON_FIRE2].down = true;
        nextKeys.keys[BUTTON_FIRE3].down = true;
        nextKeys.keys[BUTTON_FIRE4].down = true;
      }
    } else {
      if(frameNumber % 2) {
        if(player.setting_axis_current == 0)
          nextKeys.menu.x = 1.0;
        if(player.setting_axis_current == 1)
          nextKeys.menu.y = 1.0;
        if(player.setting_axis_current == 2)
          nextKeys.keys[BUTTON_ACCEPT].down = true;
      }
    }*/
  } else if(player.settingmode == SETTING_TEST) {
    nextKeys.keys[BUTTON_CANCEL].down = true;
  } else if(player.settingmode == SETTING_READY) {
    CHECK(player.setting_axistype == KSAX_ABSOLUTE);
    nextKeys.keys[BUTTON_ACCEPT].down = frameNumber % 2;
  } else {
    CHECK(0);
  }
}

void Ai::updateTween(bool live, bool pending, Coord2 playerpos, bool shopped, Coord2 joinrange, Coord2 fullshoprange, Coord2 quickshoprange, Coord2 donerange) {
  updateKeys(CORE);
  
  zeroNextKeys();
  
  if(live && shoptarget == 0)
    shoptarget = -1;
  
  if(shoptarget == -1) {
    nextKeys.menu.x = -1;
    if(!live)
      shoptarget = 0;
    else if(rng.frand() < 0.25)
      shoptarget = 1;
    else
      shoptarget = 2;
  }
  /*
  if(pending) {
    nextKeys.keys[BUTTON_CANCEL].down = (rng.frand() < 0.01);
    return;
  }*/
  
  Coord2 approach;
  if(shopped) {
    approach = donerange;
  } else if(shoptarget == 0) {
    approach = joinrange;
  } else if(shoptarget == 1) {
    approach = fullshoprange;
  } else if(shoptarget == 2) {
    approach = quickshoprange;
  } else {
    CHECK(0);
  }
  
  if(len(playerpos - approach) < 2) {
    nextKeys.keys[BUTTON_ACCEPT].down = frameNumber % 2;
    if(shoptarget == 0)
      shoptarget = -1;
    return;
  }
  
  if(playerpos.x < approach.x)
    nextKeys.menu.x = 1;
  if(playerpos.x > approach.x)
    nextKeys.menu.x = -1;
  // we just sort of ignore the y
}

Controller makeController(Coord x, Coord y, bool key, bool toggle) {
  Controller rv;
  rv.keys.resize(BUTTON_LAST);
  rv.menu = Coord2(x, y);
  for(int i = 0; i < BUTTON_LAST; i++)
    rv.keys[i].down = 0;
  rv.keys[BUTTON_FIRE1].down = key;
  rv.keys[BUTTON_ACCEPT].down = key;  // it is truly best not to ask
  rv.keys[BUTTON_CANCEL].down = toggle;
  return rv;
}

void doMegaEnumWorker(const HierarchyNode &rt, vector<pair<Money, vector<Controller> > > *weps, vector<pair<Money, vector<Controller> > > *upgs, vector<vector<Controller> > *equips, vector<Controller> *done, vector<Controller> path, const Player *player) {
  if(rt.type == HierarchyNode::HNT_CATEGORY || rt.type == HierarchyNode::HNT_EQUIP) {
    path.push_back(makeController(1, 0, false, false));
    for(int i = 0; i < rt.branches.size(); i++) {
      if(i)
        path.push_back(makeController(0, -1, false, false));
      doMegaEnumWorker(rt.branches[i], weps, upgs, equips, done, path, player);
    }
  } else if(rt.type == HierarchyNode::HNT_WEAPON) {
    weps->push_back(make_pair(player->adjustWeapon(rt.weapon).cost(rt.weapon->quantity), path));
  } else if(rt.type == HierarchyNode::HNT_UPGRADE) {  // TODO: don't buy stuff if you already have it :)
    upgs->push_back(make_pair(player->adjustUpgradeForCurrentTank(rt.upgrade).cost(), path));
  } else if(rt.type == HierarchyNode::HNT_GLORY) {
    //if(!player->hasGlory(rt.glory))
      //for(int i = 0; i < 20; i++)
        upgs->push_back(make_pair(player->adjustGlory(rt.glory).cost(), path));
  } else if(rt.type == HierarchyNode::HNT_BOMBARDMENT) {
    upgs->push_back(make_pair(player->adjustBombardment(rt.bombardment, 0).cost(), path));
  } else if(rt.type == HierarchyNode::HNT_TANK) {
    upgs->push_back(make_pair(player->adjustTankWithInstanceUpgrades(rt.tank).cost(), path));
  } else if(rt.type == HierarchyNode::HNT_EQUIPWEAPON) {
    equips->push_back(path);  // this is kind of glitchy since it won't adjust after the player buys weapons, but whatever
  } else if(rt.type == HierarchyNode::HNT_IMPLANTSLOT) {
    upgs->push_back(make_pair(player->adjustImplantSlot(rt.implantslot).cost(), path));
  } else if(rt.type == HierarchyNode::HNT_IMPLANTITEM_EQUIP) {
    upgs->push_back(make_pair(Money(0), path));
  } else if(rt.type == HierarchyNode::HNT_IMPLANTITEM_UPG) {
    upgs->push_back(make_pair(player->adjustImplant(rt.implantitem).costToLevel(player->implantLevel(rt.implantitem)), path));
  } else if(rt.type == HierarchyNode::HNT_SELL) {
  } else if(rt.type == HierarchyNode::HNT_EQUIPCATEGORY) {
  } else if(rt.type == HierarchyNode::HNT_BONUSES) {
  } else if(rt.type == HierarchyNode::HNT_IMPLANTITEM) {
  } else if(rt.type == HierarchyNode::HNT_DONE) {
    CHECK(done->size() == 0);
    *done = path;
  } else {
    dprintf("what %d\n", rt.type);
    CHECK(0);
  }
}

void doMegaEnum(const HierarchyNode &rt, vector<pair<Money, vector<Controller> > > *weps, vector<pair<Money, vector<Controller> > > *upgs, vector<vector<Controller> > *equips, vector<Controller> *done, const Player *player) {
  for(int i = 0; i < rt.branches.size(); i++) {
    vector<Controller> tvd;
    for(int j = 0; j < i; j++)
      tvd.push_back(makeController(0, -1, false, false));
    doMegaEnumWorker(rt.branches[i], weps, upgs, equips, done, tvd, player);
  }
}

Controller reversecontroller(const Controller &in, bool restrictkeys = true) {
  if(restrictkeys)
    for(int i = 0; i < in.keys.size(); i++)
      CHECK(!in.keys[i].down);
  return makeController(-in.menu.x, -in.menu.y, false, false);
}

vector<Controller> reversecontroller(const vector<Controller> &in, bool restrictkeys = true) {
  vector<Controller> rv;
  for(int i = 0; i < in.size(); i++)
    rv.push_back(reversecontroller(in[i], restrictkeys));
  reverse(rv.begin(), rv.end());
  return rv;
}

vector<Controller> makeComboToggle(const vector<Controller> &src, Rng *rng) {
  vector<Controller> oot = src;
  
  for(int i = 0; i < rng->frand() * 4; i++) {  // yes, the rng frand gets called each time
    Controller rv;
    rv.keys.resize(BUTTON_LAST);
    rv.menu = Coord2(0, 0);
    for(int i = 0; i < BUTTON_LAST; i++)
      rv.keys[i].down = 0;
    if(rng->frand() < 0.1) {
      vector<Controller> path;
      
      rv.keys[BUTTON_ACCEPT].down = true;
      rv.menu.y = rng->frand() * 2 - 1;
      path.push_back(rv);
      rv.keys[BUTTON_ACCEPT].down = false;
      
      float bias = rng->frand();
      for(int i = 0; i < rng->frand() * 100; i++) {
        if(rng->frand() < bias) {
          rv.menu.y = 1;
        } else {
          rv.menu.y = -1;
        }
        path.push_back(rv);
      }
      rv.menu.y = rng->frand() * 2 - 1;
      rv.keys[BUTTON_ACCEPT].down = true;
      path.push_back(rv);
      
      oot.insert(oot.end(), path.begin(), path.end());
      vector<Controller> rev = reversecontroller(path, false);
      for(int i = 0; i < rev.size(); i++)
        rev[i].keys[BUTTON_ACCEPT].down = false;
      oot.insert(oot.end(), rev.begin(), rev.end());
    } else {
      rv.keys[BUTTON_FIRE1].down = rng->frand() < 0.5;
      rv.keys[BUTTON_FIRE2].down = rng->frand() < 0.5;
      rv.keys[BUTTON_FIRE3].down = rng->frand() < 0.5;
      rv.keys[BUTTON_FIRE4].down = rng->frand() < 0.5;
      oot.push_back(rv);
    }
  }

  vector<Controller> rev = reversecontroller(src);
  oot.insert(oot.end(), rev.begin(), rev.end());

  return oot;
}


vector<Controller> makeComboAppend(const vector<Controller> &src, int count) {
  vector<Controller> oot;
  oot.insert(oot.end(), src.begin(), src.end());
  for(int i = 0; i < count; i++)
    oot.push_back(makeController(0, 0, true, false));
  vector<Controller> rev = reversecontroller(src);
  oot.insert(oot.end(), rev.begin(), rev.end());
  return oot;
}

void Ai::updateShop(const Player *player, const HierarchyNode &hierarchy, bool athead) {
  updateKeys(CORE);
  
  zeroNextKeys();
  if(shopQueue.size()) {
    nextKeys = shopQueue[0];
    shopQueue.pop_front();
    return;
  }
  if(shopdone) {  // If our shop is done, something weird has happened.
    CHECK(player->blockedReasons().size()); // It's possible the player isn't allowed to continue for some reason.
    CHECK(shopQueue.size() == 0);
    shopdone = false;
    // Next steps: wait to avoid repeat glitches, move down, wait again.
    nextKeys = makeController(0, 0, false, false);
    shopQueue.push_back(makeController(0, -1, false, false));
    shopQueue.push_back(makeController(0, 0, false, false));
    return;
  }
  StackString sst("Creating shop");
  CHECK(!shopdone);
  CHECK(athead);
  vector<pair<Money, vector<Controller> > > weps;
  vector<pair<Money, vector<Controller> > > upgs;
  vector<vector<Controller> > equips;
  vector<Controller> done;
  doMegaEnum(hierarchy, &weps, &upgs, &equips, &done, player);
  dprintf("%d weps, %d upgs, %d equips, %d donesize\n", weps.size(), upgs.size(), equips.size(), done.size());
  CHECK(weps.size());
  CHECK(done.size());
  sort(weps.begin(), weps.end());
  sort(upgs.begin(), upgs.end());
  Money upgcash = player->getCash() / 2;
  Money weapcash = player->getCash();
  vector<vector<Controller> > commands;
  
  if(rng.frand() > (float)equips.size() / 10) {
    dprintf("%d equips, buying more\n", equips.size());
    {
      float cullperc = 0.9 + rng.frand() * 0.1;
      float singleperc = rng.frand() + 0.5;
      while(upgcash > Money(0) && upgs.size()) {
        int dlim = int(upgs.size() * rng.frand());
        if(rng.frand() < cullperc) {
          upgs.erase(upgs.begin() + dlim);
          continue;
        }
        upgcash -= upgs[dlim].first;
        weapcash -= upgs[dlim].first;
        commands.push_back(makeComboAppend(upgs[dlim].second, 1));
        if(rng.frand() < singleperc)
          upgs.erase(upgs.begin() + dlim);
      }
    }
    
    for(int i = 0; i < 5; i++) {
      if(weapcash > Money(0)) {
        int dlim = 0;
        while(dlim < weps.size() && weps[dlim].first <= weapcash)
          dlim++;
        dlim = int(dlim * rng.frand());
        int amount = 1;
        if(weps[dlim].first > Money(0))
          amount = min(weapcash / weps[dlim].first, 100);
        weapcash -= weps[dlim].first * amount;
        commands.push_back(makeComboAppend(weps[dlim].second, amount));
      }
    }
  }
  
  {
    float cullperc = rng.frand();
    float singleperc = rng.frand() + 0.5;
    while(equips.size()) {
      int dite = int(equips.size() * rng.frand());
      if(rng.frand() < cullperc) {
        equips.erase(equips.begin() + dite);
        continue;
      }
      commands.push_back(makeComboToggle(equips[dite], &rng));
      if(rng.frand() < singleperc)
        equips.erase(equips.begin() + dite);
    }
  }
  
  random_shuffle(commands.begin(), commands.end());
  
  list<Controller> shoq;
  for(int i = 0; i < commands.size(); i++)
    shoq.insert(shoq.end(), commands[i].begin(), commands[i].end());
  shoq.insert(shoq.end(), done.begin(), done.end());
  shoq.push_back(makeController(0, 0, true, false));
  
  for(list<Controller>::iterator it = shoq.begin(); it != shoq.end(); ) {
    
    list<Controller>::iterator nit = it;
    nit++;
    if(nit == shoq.end())
      break;
    list<Controller>::iterator tit = nit;
    tit++;
    if(tit == shoq.end())
      break;
    
    bool hasbuttons = false;
    for(int i = 0; i < nit->keys.size(); i++)
      if(nit->keys[i].down)
        hasbuttons = true;
    for(int i = 0; i < tit->keys.size(); i++)
      if(tit->keys[i].down)
        hasbuttons = true;
    
    if(hasbuttons) {
      it++;
      continue;
    }
    
    if(reversecontroller(*nit) == *tit) {
      shoq.erase(nit);
      shoq.erase(tit);
    } else {
      it++;
    }
  };
  
  {
    StackString sts("Final pass and padding");
    for(list<Controller>::iterator it = shoq.begin(); it != shoq.end(); it++) {
      for(int k = 0; k < 1; k++)
        shopQueue.push_back(makeController(0, 0, false, false));
      shopQueue.push_back(*it);
    }
    shopdone = true;
    updateShop(player, hierarchy, false);
    dprintf("shop prepared, %d moves\n", shopQueue.size());
  }
}

GameAi *Ai::getGameAi() {
  updateKeys(GAME);
  
  shopdone = false;
  shoptarget = -1;
  return &gai;
}

void Ai::updateWaitingForReport() {
  updateKeys(CORE);
  
  zeroNextKeys();
  nextKeys.menu = Coord2(0, 0);
  nextKeys.keys[0].down = frameNumber % 2;
}

void Ai::updateGameEnd() {
  updateKeys(CORE);
  
  zeroNextKeys();
  nextKeys.menu = Coord2(0, 0);
  nextKeys.keys[0].down = frameNumber % 2;
}

Controller Ai::getNextKeys() const {
  if(curframe != frameNumber)
    dprintf("%d, %d\n", curframe, frameNumber);
  CHECK(curframe == frameNumber);
  
  if(source == CORE) {
    Controller cnm = nextKeys;
    cnm.menu = clamp(cnm.menu, Coord4(-1, -1, 1, 1));
    return cnm;
  } else if(source == GAME) {
    Keystates kst = gai.getNextKeys();
    Controller kont;
    kont.keys.resize(BUTTON_LAST);
    kont.menu = kst.udlrax;
    kont.keys[BUTTON_ACCEPT] = kst.accept;
    kont.keys[BUTTON_CANCEL] = kst.cancel;
    for(int i = 0; i < SIMUL_WEAPONS; i++)
      kont.keys[BUTTON_FIRE1 + i] = kst.fire[i];
    kont.menu = clamp(kont.menu, Coord4(-1, -1, 1, 1));
    return kont;
  } else {
    CHECK(0);
  }
}

Ai::Ai() : rng(unsync().generate_seed()), gai(&rng) {
  nextKeys.keys.resize(BUTTON_LAST);
  shopdone = false;
  source = UNKNOWN;
  curframe = -1;
  shoptarget = -1;
}

void Ai::zeroNextKeys() {
  nextKeys.menu = Coord2(0, 0);
  for(int i = 0; i < nextKeys.keys.size(); i++)
    nextKeys.keys[i].down = false;
}

void Ai::updateKeys(int desiredsource) {
  if(curframe >= frameNumber)
    dprintf("Weird frame inconsistency in AI on frame %d\n", curframe);
  curframe = frameNumber;
  source = desiredsource;
}
