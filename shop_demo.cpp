
#include "shop_demo.h"

#include "debug.h"
#include "game_ai.h"
#include "game_tank.h"
#include "gfx.h"
#include "player.h"
#include "args.h"

using namespace std;

enum {DEMOMODE_FIRINGRANGE, DEMOMODE_MINE, DEMOMODE_BOMBARDMENT, DEMOMODE_GLORY, DEMOMODE_LAST};

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
  
  Rng rng;
  
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
  
  GameAiScatterbombing(int in_targetid, float in_rad, RngSeed seed) : rng(seed) { targetid = in_targetid; rad = in_rad; ready = false; bombing = true; }
};

class GameAiKamikaze : public GameAi {
  int targetid;
  float rad;
  float lastrad;
  
  bool ready;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    float dist = len(players[targetid].pos - players[me].pos).toFloat();
    if(dist <= rad || rad > lastrad || abs(dist - lastrad) < 0.01) {
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
    lastrad = 1e9;
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
    Coord angdif = players[me].d - getAngle(players[me].pos) - COORDPI / 2;
    while(angdif < COORDPI) angdif += COORDPI * 2;
    while(angdif > COORDPI) angdif -= COORDPI * 2;
    if(angdif < Coord(-0.1))
      nextKeys.udlrax.x += 0.5;
    if(angdif > Coord(0.1))
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

class GameAiMining : public GameAi {
  int direction;
  int frames_to_start;
  
  Rng rng;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    if(direction == 2) {
      direction = -((players[me].pos.x) / abs(players[me].pos.x)).toInt();
      CHECK(abs(direction) == 1);
      frames_to_start = int(rng.frand() * 30);
    }
    if(direction == 0)
      return;
    if(players[me].pos.x * direction > 60) {
      direction = 0;
      return;
    }
    nextKeys.udlrax.x = direction;
    nextKeys.fire[0].down = (frames_to_start <= 0);
    frames_to_start--;
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
  
public:
  bool running() const { return direction; };
  void start() { direction = 2; };
  GameAiMining(RngSeed seed) : rng(seed) { direction = 0; frames_to_start = 0; }
};

class GameAiTraversing : public GameAi {
  Coord2 destination;
  bool active;
  bool starting;
  
  Rng rng;
  
  void updateGameWork(const vector<Tank> &players, int me) {
    if(starting) {
      starting = false;
      destination = Coord2(Coord(rng.frand() * 80 - 40), abs(players[me].pos.y) / players[me].pos.y * -60);
      active = true;
    }
    if(len(players[me].pos - destination) < 1) {
      active = false;
    }
    if(!active)
      return;
    nextKeys.udlrax = normalize(destination - players[me].pos).toFloat();
    nextKeys.udlrax.y *= -1;
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
  
public:
  bool running() const { return active; };
  void start() { starting = true; };
  GameAiTraversing(RngSeed seed) : rng(seed) { active = false; starting = false; }
};

const float weapons_xpses_normal[] = { -80, -80, 0, 0, 80, 80 };
const float weapons_ypses_normal[] = { 120, -120, 120, -40, 120, 40 };
const float weapons_xpses_melee[] = { -20, -20, 0, 0, 20, 20 };
const float weapons_ypses_melee[] = { 6, -10, 6, -4, 6, 6 };
const float weapons_facing[] = { PI * 3 / 2, PI / 2, PI * 3 / 2, PI / 2, PI * 3 / 2, PI / 2 };
const float weapons_facing_back[] = { PI / 2, PI / 2, PI / 2, PI / 2, PI / 2, PI / 2 };
const int weapons_teams[] = { 0, 1, 0, 1, 0, 1 };
const int weapons_mode[] = { DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_QUIET, DEMOPLAYER_DPS };
const int weapons_progression[] = { 600, 0 };

const float mines_xpses[] = { 0, -60, -60, -60 };
const float mines_ypses[] = { -60, -40, 0, 40 };
const float mines_facing[] = { PI / 2, 0, 0, 0 };
const int mines_teams[] = { 0, 1, 1, 1 };
const int mines_mode[] = { DEMOPLAYER_DPH, DEMOPLAYER_QUIET, DEMOPLAYER_QUIET, DEMOPLAYER_QUIET };
const int mines_progression[] = { 600, 0 };

const float bombardment_xpses[] = { -30, -30, 30, 30, -30, -30, 30, 30 };
const float bombardment_ypses[] = { -30, -30, -30, -30, 30, 30, 30, 30 };
const int bombardment_teams[] = { 0, 1, 0, 1, 0, 1, 0, 1 };
const int bombardment_mode[] = { DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT };
const int bombardment_progression[] = { 6000, 0 };

const float glory_xpses[] = { -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5 };
const float glory_ypses[] = { -0.5, -0.25, -0.5, -0.25, 0.5, 0.25, 0.5, 0.25 };
const int glory_teams[] = { 0, 1, 0, 1, 0, 1, 0, 1 };
const int glory_mode[] = { DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET, DEMOPLAYER_DPC, DEMOPLAYER_QUIET };
const int glory_progression[] = { 6000, 0 };

void ShopDemo::init(const IDBWeapon *weap, const Player *player, Recorder *recorder) {
  StackString sst("Initting demo weapon shop");
  identifier = "weapon " + nameFromIDB(weap) + " with tank " + nameFromIDB(player->getTank().base());
  
  int primary = -1;
  
  if(weap->launcher->demomode == WDM_FIRINGRANGE || weap->launcher->demomode == WDM_BACKRANGE) {
    mode = DEMOMODE_FIRINGRANGE;
    game.players.clear();
    game.players.resize(6, *player);
    CHECK(factionList().size() >= game.players.size());
    for(int i = 0; i < game.players.size(); i++) {
      game.players[i].forceAcquireWeapon(weap, 1000000);
      if(!game.players[i].hasValidTank())
        game.players[i].forceAcquireTank(defaultTank());
    }
    
    ais.clear();
    for(int i = 0; i < 3; i++) {
      ais.push_back(smart_ptr<GameAi>(new GameAiFiring));
      
      game.players[i * 2 + 1].forceAcquireTank(defaultTank());
      ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    }
    
    const float *facing;
    if(weap->launcher->demomode == WDM_FIRINGRANGE)
      facing = weapons_facing;
    else if(weap->launcher->demomode == WDM_BACKRANGE)
      facing = weapons_facing_back;
    else
      CHECK(0);
    
    if(weap->launcher->firingrange_distance == WFRD_NORMAL) {
      game.game.initDemo(&game.players, 160, weapons_xpses_normal, weapons_ypses_normal, facing, weapons_teams, weapons_mode, false, Float2(5, 5), recorder);
    } else if(weap->launcher->firingrange_distance == WFRD_MELEE) {
      CHECK(ARRAY_SIZE(weapons_facing) == 6);
      Tank tankmajor;
      Tank tankminor;
      tankmajor.init(game.players[0].getTank(), Color(1, 1, 1));
      tankminor.init(game.players[1].getTank(), Color(1, 1, 1));
      Float2 basetest(weapons_xpses_melee[5], weapons_ypses_melee[5]);
      float dist_min = 0.1;
      float dist_max = 10;
      while(dist_max - dist_min > 0.01) {
        float dist_cen = (dist_min + dist_max) / 2;
        Float2 pos = basetest + makeAngle(-PI / 4) * dist_cen;
        if(getPathRelation(tankmajor.getTankVertices(Coord2(weapons_xpses_melee[4], weapons_ypses_melee[4]), Coord(facing[4])), tankminor.getTankVertices(Coord2(pos), Coord(weapons_facing[5]))) == PR_INTERSECT) {
          dist_min = dist_cen;
        } else {
          dist_max = dist_cen;
        }
      }
      
      dprintf("dist_max is %f\n", dist_max);
      dist_max += 0.1;
      
      CHECK(getPathRelation(tankminor.getTankVertices(Coord2(weapons_xpses_melee[4], weapons_ypses_melee[4]), Coord(facing[4])), tankminor.getTankVertices(Coord2(basetest + makeAngle(-PI / 4) * dist_max), Coord(weapons_facing[5]))) != PR_INTERSECT);
      
      float wxm[ARRAY_SIZE(weapons_xpses_melee)];
      float wym[ARRAY_SIZE(weapons_xpses_melee)];
      memcpy(wxm, weapons_xpses_melee, sizeof(wxm));
      memcpy(wym, weapons_ypses_melee, sizeof(wym));
      Float2 newpos = basetest + makeAngle(-PI / 4) * dist_max;
      wxm[5] = newpos.x;
      wym[5] = newpos.y;
      game.game.initDemo(&game.players, 40, wxm, wym, facing, weapons_teams, weapons_mode, false, Float2(-10, 8), recorder); // this is kind of painful
    } else {
      CHECK(0);
    }
    
    progression = weapons_progression;
    primary = 0;
  } else if(weap->launcher->demomode == WDM_MINES) {
    mode = DEMOMODE_MINE;
    game.players.clear();
    game.players.resize(ARRAY_SIZE(mines_xpses), *player);
    CHECK(factionList().size() >= game.players.size());
    for(int i = 0; i < game.players.size(); i++) {
      game.players[i].forceAcquireWeapon(weap, 1000000);
      if(!game.players[i].hasValidTank())
        game.players[i].forceAcquireTank(defaultTank());
    }
    
    ais.clear();  
    mine_miners.clear();
    {
      GameAiTraversing *gat = new GameAiTraversing(unsync().generate_seed());
      ais.push_back(smart_ptr<GameAi>(gat));
      mine_traverser = gat;
      game.players[0].forceAcquireTank(defaultTank());
    }
    for(int i = 1; i < game.players.size(); i++) {
      GameAiMining *gam = new GameAiMining(unsync().generate_seed());
      ais.push_back(smart_ptr<GameAi>(gam));
      mine_miners.push_back(gam);
    }
    
    game.game.initDemo(&game.players, 100, mines_xpses, mines_ypses, mines_facing, mines_teams, mines_mode, false, Float2(5, 5), recorder);
    mine_mined = false;
    
    progression = mines_progression;
    primary = 1;
  } else {
    CHECK(0);
  }
  
  CHECK(primary != -1);
  if(hasShopcache(weap)) {
    prerolled = true;
    game.runShopcache(getShopcache(weap), primary);
  } else {
    prerolled = false;
  }
};

void ShopDemo::init(const IDBBombardment *bombard, const Player *player, Recorder *recorder) {
  StackString sst("Initting demo bombardment shop");
  identifier = "bombardment " + nameFromIDB(bombard);
  mode = DEMOMODE_BOMBARDMENT;
  
  game.players.clear();
  game.players.resize(8, *player);
  CHECK(factionList().size() >= game.players.size());
  for(int i = 0; i < game.players.size(); i++) {
    game.players[i].forceAcquireBombardment(bombard);
    if(!game.players[i].hasValidTank())
      game.players[i].forceAcquireTank(defaultTank());
  }
  
  ais.clear();
  bombardment_scatterers.clear();
  for(int i = 0; i < 4; i++) {
    game.players[i * 2].forceAcquireTank(defaultTank());
    ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    
    GameAiScatterbombing *gas = new GameAiScatterbombing(i * 2, pow((float)2, (float)i + 1), unsync().generate_seed());
    ais.push_back(smart_ptr<GameAi>(gas));
    bombardment_scatterers.push_back(gas);
  }
  
  game.game.initDemo(&game.players, 50, bombardment_xpses, bombardment_ypses, NULL, bombardment_teams, bombardment_mode, false, Float2(-10, 8), recorder);
  
  progression = bombardment_progression;
  
  if(hasShopcache(bombard)) {
    prerolled = true;
    game.runShopcache(getShopcache(bombard), 1);
  } else {
    prerolled = false;
  }
};

void ShopDemo::init(const IDBGlory *glory, const Player *player, Recorder *recorder) {
  StackString sst("Initting demo glory shop");
  identifier = "glory " + nameFromIDB(glory);
  mode = DEMOMODE_GLORY;
  
  game.players.clear();
  game.players.resize(8, *player);
  CHECK(factionList().size() >= game.players.size());
  for(int i = 0; i < game.players.size(); i++) {
    game.players[i].forceAcquireGlory(glory);
    if(!game.players[i].hasValidTank())
      game.players[i].forceAcquireTank(defaultTank());
  }
  
  ais.clear();
  glory_kamikazes.clear();
  for(int i = 0; i < 4; i++) {
    game.players[i * 2].forceAcquireTank(defaultTank());
    ais.push_back(smart_ptr<GameAi>(new GameAiNull));
    
    GameAiKamikaze *gas = new GameAiKamikaze(i * 2, i ? i * 5 + 5 : 0);
    ais.push_back(smart_ptr<GameAi>(gas));
    glory_kamikazes.push_back(gas);
  }
  respawn = false;
  
  {
    float xps[ARRAY_SIZE(glory_xpses)];
    float yps[ARRAY_SIZE(glory_ypses)];
    for(int i = 0; i < ARRAY_SIZE(xps); i++) {
      xps[i] = glory->demo_range * glory_xpses[i];
      yps[i] = glory->demo_range * glory_ypses[i];
    }
    
    Player player;
    
    game.game.initDemo(&game.players, glory->demo_range, xps, yps, NULL, glory_teams, glory_mode, true, Float2(5, 5), recorder);
  }
  
  progression = glory_progression;
  
  glory_respawnPlayers();
  
  if(hasShopcache(glory)) {
    prerolled = true;
    game.runShopcache(getShopcache(glory), 1);
  } else {
    prerolled = false;
  }
};

void ShopDemo::glory_respawnPlayers() {
  for(int i = 0; i < game.players.size(); i += 2) {
    Coord2 pos = game.game.queryPlayerLocation(i);
    Coord facing = Coord(unsync().frand()) * 2 * COORDPI;
    pos += makeAngle(facing) * 40;
    game.game.respawnPlayer(i + 1, pos, facing + COORDPI);
  }
};

DEFINE_bool(fastForwardOnNoCache, true, "Enables fastforwarding when there's no shopcache file to fall back on");

int ShopDemo::getMultiplier() const {
  if(!FLAGS_fastForwardOnNoCache || prerolled)
    return 1;
  
  if(game.game.frameCount() < progression[0])
    return 10;
  if(game.game.frameCount() < progression[1])
    return 4;
  return 1;
}

void ShopDemo::runSingleTick() {
  StackString sst("Demo tick " + identifier);
  if(mode == DEMOMODE_FIRINGRANGE) {
  } else if(mode == DEMOMODE_MINE) {
    if(!mine_mined) {
      if(!mine_traverser->running()) {
        for(int i = 0; i < mine_miners.size(); i++)
          mine_miners[i]->start();
        mine_mined = true;
      }
    } else {
      bool srun = false;
      for(int i = 0; i < mine_miners.size(); i++)
        if(mine_miners[i]->running())
          srun = true;
      if(!srun) {
        mine_traverser->start();
        mine_mined = false;
      }
    }
  } else if(mode == DEMOMODE_BOMBARDMENT) {
    bool notready = false;
    for(int i = 0; i < bombardment_scatterers.size(); i++)
      if(!bombardment_scatterers[i]->readytofire())
        notready = true;
    if(!notready) {
      for(int i = 0; i < bombardment_scatterers.size(); i++)
        bombardment_scatterers[i]->bombsaway();
      game.game.addStatCycle();
    }
  } else if(mode == DEMOMODE_GLORY) {
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
          game.game.kill(i * 2 + 1);
        }
        game.game.addStatCycle();
        respawn = true;
      }
    }
  } else {
    CHECK(0);
  }
  
  vector<GameAi *> tai;
  for(int i = 0; i < ais.size(); i++)
    tai.push_back(ais[i].get());
  game.game.ai(tai);
  
  vector<Keystates> kist;
  for(int i = 0; i < tai.size(); i++)
    kist.push_back(tai[i]->getNextKeys());
  game.runTick(kist, &unsync());
};

void ShopDemo::runTick() {
  int timing = getMultiplier();
  for(int i = 0; i < timing; i++) {
    runSingleTick();
  }
};

void ShopDemo::renderFrame() const {
  game.renderToScreen();
  if(getMultiplier() != 1) {
    setZoom(Float4(0, 0, 1, 1));
    setColor(1, 1, 1);
    drawJustifiedText(StringPrintf("Fastforward %dx", getMultiplier()), 0.05, Float2(0, 1), TEXT_MIN, TEXT_MAX);
  }
};

vector<float> ShopDemo::getStats() const {
  vector<pair<Float2, pair<float, string> > > stats = game.game.getStats();
  vector<float> rv;
  for(int i = 0; i < stats.size(); i++)
    rv.push_back(stats[i].second.first);
  return rv;
}

void ShopDemo::dumpMetastats(Recorder *recorder) const {
  game.game.dumpMetastats(recorder);
}

ShopDemo::ShopDemo() { identifier = "uninitialized"; };
ShopDemo::~ShopDemo() { };
