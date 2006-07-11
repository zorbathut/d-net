
#include "shop_demo.h"

#include "gfx.h"
#include "debug.h"
#include "game_ai.h"

enum {DEMOMODE_WEAPON, DEMOMODE_BOMBARDMENT, DEMOMODE_LAST};

class GameAiNull : public GameAi {
private:
  void updateGameWork(const vector<Tank> &players, int me) {
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
};

class GameAiFiring : public GameAi {
private:
  void updateGameWork(const vector<Tank> &players, int me) {
    nextKeys.fire[0].down = true;
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
};

Float2 randomDisc(Rng &rng) {
  Float2 rv;
  while(1) {
    rv.x = rng.frand();
    rv.y = rng.frand();
    if(len(rv) > 1)
      continue;
    if(rng.frand() < 0.5)
      rv.x *= -1;
    if(rng.frand() < 0.5)
      rv.y *= -1;
    return rv;
  }
}

class GameAiScatterbombing : public GameAi {
  int targetid;
  float rad;
  
  Coord2 target;
  
  bool ready;
  bool bombing;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    CHECK(0);
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    if(ready == false && bombing == true) {
      target = players[targetid].pos + Coord2(randomDisc(rng) * rad);
      bombing = false;
    }
    
    Coord2 dir = target - mypos;
    //if(targetid == 6)
      //dprintf("%f\n", len(dir).toFloat());
    if(len(dir) > 1)
      dir = normalize(dir);
    nextKeys.udlrax = dir.toFloat();
    nextKeys.udlrax.y *= -1;
    if(len(target - mypos).toFloat() < 0.2) {
      ready = true;
    }
    if(bombing) {
      nextKeys.fire[0].down = true;
      ready = false;
      bombing = false;
      target = players[targetid].pos + Coord2(randomDisc(rng) * rad);
    }
  }
  
public:
  bool readytofire() const {
    return ready;
  }
  void bombsaway() {
    bombing = true;
  }
  
  GameAiScatterbombing(int in_targetid, float in_rad) { targetid = in_targetid; rad = in_rad; ready = false; bombing = true; }
};

const float weapons_xpses[] = { -80, -80, 0, 0, 80, 80 };
const float weapons_ypses[] = { 120, -80, 120, -40, 120, 40 };
const int weapons_mode[] = { DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS };
const int weapons_progression[] = { 60, 600 };

const float bombardment_xpses[] = { -30, -30, 30, 30, -30, -30, 30, 30 };
const float bombardment_ypses[] = { -30, -30, -30, -30, 30, 30, 30, 30 };
const int bombardment_mode[] = { DEMOPLAYER_DPH, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPH, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPH, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPH, DEMOPLAYER_BOMBSIGHT };
const int bombardment_progression[] = { 6000, 1200 };


void ShopDemo::init(const IDBWeapon *weap, const Player *player) {
  StackString sst("Initting demo weapon shop");
  mode = DEMOMODE_WEAPON;
  
  players.clear();
  players.resize(6);
  CHECK(factionList().size() >= players.size());
  for(int i = 0; i < players.size(); i++) {
    players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode
    players[i].addCash(Money(1000000000));
    players[i].forceAcquireWeapon(weap, 1000000);
  }
  
  ais.clear();
  for(int i = 0; i < 3; i++) {
    ais.push_back(smart_ptr<GameAi>(new GameAiFiring));
    ais.push_back(smart_ptr<GameAi>(new GameAiNull));
  }
  
  game.initDemo(&players, 160, weapons_xpses, weapons_ypses, weapons_mode);
  
  progression = weapons_progression;
};

void ShopDemo::init(const IDBBombardment *bombard, const Player *player) {
  StackString sst("Initting demo bombardment shop");
  mode = DEMOMODE_BOMBARDMENT;
  
  players.clear();
  players.resize(8);
  CHECK(factionList().size() >= players.size());
  for(int i = 0; i < players.size(); i++) {
    players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode and the right faction data and the right tanks and upgrades and so forth
    players[i].addCash(Money(1000000000));
    players[i].forceAcquireBombardment(bombard);
  }
  
  ais.clear();
  bombardment_scatterers.clear();
  for(int i = 0; i < 4; i++) {
    ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    GameAiScatterbombing *gas = new GameAiScatterbombing(i * 2, pow((float)2, (float)i + 1));
    ais.push_back(smart_ptr<GameAi>(gas));
    bombardment_scatterers.push_back(gas);
  }
  
  game.initDemo(&players, 50, bombardment_xpses, bombardment_ypses, bombardment_mode);
  
  progression = bombardment_progression;
};

int mult(int frams, const int *progression) {
  if(frams < progression[0])
    return 10;
  if(frams < progression[1])
    return 4;
  return 1;
}

void ShopDemo::runTick() {
  vector<GameAi *> tai;
  for(int i = 0; i < ais.size(); i++)
    tai.push_back(ais[i].get());
  for(int i = 0; i < mult(game.frameCount(), progression); i++) {
    if(mode == DEMOMODE_BOMBARDMENT) {
      bool notready = false;
      for(int i = 0; i < bombardment_scatterers.size(); i++)
        if(!bombardment_scatterers[i]->readytofire())
          notready = true;
      if(!notready) {
        for(int i = 0; i < bombardment_scatterers.size(); i++)
          bombardment_scatterers[i]->bombsaway();
        game.addStatHit();
      }
    }
    
    game.ai(tai);
    vector<Keystates> kist;
    for(int i = 0; i < tai.size(); i++)
      kist.push_back(tai[i]->getNextKeys());
    game.runTick(kist);
  }
};

void ShopDemo::renderFrame() const {
  game.renderToScreen();
  setZoom(0, 0, 1);
  setColor(1, 1, 1);
  if(mult(game.frameCount(), progression) != 1)
    drawJustifiedText(StringPrintf("%dx", mult(game.frameCount(), progression)), 0.1, 0, 1, TEXT_MIN, TEXT_MAX);
};

ShopDemo::ShopDemo() { };
ShopDemo::~ShopDemo() { };
