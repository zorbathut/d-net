
#include "shop_demo.h"

#include "gfx.h"
#include "debug.h"
#include "game_ai.h"

enum {DEMOMODE_WEAPON, DEMOMODE_BOMBARDMENT, DEMOMODE_GLORY, DEMOMODE_LAST};

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

class GameAiKamikaze : public GameAi {
  int targetid;
  float rad;
  float lastrad;
  
  bool ready;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    float dist = len(players[targetid].pos - players[me].pos).toFloat();
    if(dist <= rad || abs(dist - lastrad) < 0.0001) {
      ready = true;
    } else {
      nextKeys.udlrax.y = 1;
      nextKeys.axmode = KSAX_STEERING;
      lastrad = dist;
    }
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
  }
  
public:
  bool readytoexplode() const {
    return ready;
  }
  void exploded() {
    ready = false;
  }
  
  GameAiKamikaze(int in_targetid, float in_rad) { targetid = in_targetid; rad = in_rad; lastrad = 1e9; ready = false; }
};

class GameAiCircling : public GameAi {
  bool firing;
  float dist;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    nextKeys.axmode = KSAX_STEERING;
    nextKeys.udlrax.y = 1;
    if(len(players[me].pos).toFloat() > dist + 2)
      nextKeys.udlrax.x += 0.5;
    if(len(players[me].pos).toFloat() < dist - 2)
      nextKeys.udlrax.x += -0.5;
    float angdif = players[me].d - getAngle(players[me].pos).toFloat() - PI / 2;
    while(angdif < PI) angdif += PI * 2;
    while(angdif > PI) angdif -= PI * 2;
    if(angdif < -0.1)
      nextKeys.udlrax.x += 0.5;
    if(angdif > 0.1)
      nextKeys.udlrax.x -= 0.5;
    if(abs(nextKeys.udlrax.x) == 0.5)
      nextKeys.udlrax.x *= 1.5;
    nextKeys.fire[0].down = firing;
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
  
public:
  GameAiCircling(float in_dist, bool in_firing) { dist = in_dist; firing = in_firing; }
};

const float weapons_xpses[] = { -80, -80, 0, 0, 80, 80 };
const float weapons_ypses[] = { 120, -80, 120, -40, 120, 40 };
const int weapons_mode[] = { DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS };
const int weapons_progression[] = { 60, 600 };

const float mines_circle = 60;
const float mines_xpses[] = { 0, 0 };
const float mines_ypses[] = { -mines_circle, mines_circle };
const float mines_facing[] = { 0, PI };
const int mines_mode[] = { DEMOPLAYER_DPH, DEMOPLAYER_QUIET };
const int mines_progression[] = { 6000, 0 };

const float bombardment_xpses[] = { -30, -30, 30, 30, -30, -30, 30, 30 };
const float bombardment_ypses[] = { -30, -30, -30, -30, 30, 30, 30, 30 };
const int bombardment_mode[] = { DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT };
const int bombardment_progression[] = { 6000, 0 };

const float glory_xpses[] = { -50, -50, 50, 50, -50, -50, 50, 50 };
const float glory_ypses[] = { -50, -50, -50, -50, 50, 50, 50, 50 };
const int glory_mode[] = { DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET };
const int glory_progression[] = { 6000, 0 };

void ShopDemo::init(const IDBWeapon *weap, const Player *player) {
  StackString sst("Initting demo weapon shop");
  mode = DEMOMODE_WEAPON;
  
  if(weap->demomode == WDM_FIRINGRANGE) {
    players.clear();
    players.resize(6);
    CHECK(factionList().size() >= players.size());
    for(int i = 0; i < players.size(); i++) {
      players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode and stats
      players[i].forceAcquireWeapon(weap, 1000000);
    }
    
    ais.clear();
    for(int i = 0; i < 3; i++) {
      ais.push_back(smart_ptr<GameAi>(new GameAiFiring));
      ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    }
    
    game.initDemo(&players, 160, weapons_xpses, weapons_ypses, NULL, weapons_mode);
    
    progression = weapons_progression;
  } else if(weap->demomode == WDM_MINES) {
    players.clear();
    players.resize(2);
    CHECK(factionList().size() >= players.size());
    for(int i = 0; i < players.size(); i++) {
      players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode and stats
      players[i].forceAcquireWeapon(weap, 1000000);
    }
    
    ais.clear();  
    ais.push_back(smart_ptr<GameAi>(new GameAiCircling(mines_circle, 0)));
    ais.push_back(smart_ptr<GameAi>(new GameAiCircling(mines_circle, 1)));
    
    game.initDemo(&players, 100, mines_xpses, mines_ypses, mines_facing, mines_mode);
    
    progression = mines_progression;
  } else {
    CHECK(0);
  }
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
  
  game.initDemo(&players, 50, bombardment_xpses, bombardment_ypses, NULL, bombardment_mode);
  
  progression = bombardment_progression;
};

void ShopDemo::init(const IDBGlory *glory, const Player *player) {
  StackString sst("Initting demo glory shop");
  mode = DEMOMODE_GLORY;
  
  players.clear();
  players.resize(8);
  CHECK(factionList().size() >= players.size());
  for(int i = 0; i < players.size(); i++) {
    players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode and the right faction data and the right tanks and upgrades and so forth
    players[i].addCash(Money(1000000000));
    players[i].forceAcquireGlory(glory);
  }
  
  ais.clear();
  glory_kamikazes.clear();
  for(int i = 0; i < 4; i++) {
    ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    GameAiKamikaze *gas = new GameAiKamikaze(i * 2, i ? i * 5 + 5 : 0);
    ais.push_back(smart_ptr<GameAi>(gas));
    glory_kamikazes.push_back(gas);
  }
  respawn = false;
  
  game.initDemo(&players, 100, glory_xpses, glory_ypses, NULL, glory_mode);
  
  progression = glory_progression;
  
  glory_respawnPlayers();
};

void ShopDemo::glory_respawnPlayers() {
  for(int i = 0; i < players.size(); i += 2) {
    Coord2 pos = game.queryPlayerLocation(i);
    float facing = frand() * 2 * PI;
    Coord2 dir = makeAngle(Coord(facing));
    pos += dir * 40;
    game.respawnPlayer(i + 1, pos, facing + PI);
  }
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
        game.addStatCycle();
      }
    }
    
    if(mode == DEMOMODE_GLORY) {
      if(respawn) {
        glory_respawnPlayers();
        respawn = false;
      } else {
        bool notready = false;
        for(int i = 0; i < glory_kamikazes.size(); i++)
          if(!glory_kamikazes[i]->readytoexplode())
            notready = true;
        if(!notready) {
          for(int i = 0; i < glory_kamikazes.size(); i++) {
            glory_kamikazes[i]->exploded();
            game.kill(i * 2 + 1);
          }
          game.addStatCycle();
          respawn = true;
        }
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
