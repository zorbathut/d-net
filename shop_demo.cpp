
#include "shop_demo.h"

#include "gfx.h"
#include "debug.h"
#include "game_ai.h"

class GameAiNull : public GameAi {
public:
  void updateGame(const vector<Tank> &players, int me) {
    zeroNextKeys();
    normalizeNext();
  }
  void updateBombardment(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
};

class GameAiFiring : public GameAi {
public:
  void updateGame(const vector<Tank> &players, int me) {
    zeroNextKeys();
    nextKeys.fire[0].down = true;
    normalizeNext();
  }
  void updateBombardment(const vector<Tank> &players, Coord2 mypos) {
    CHECK(0);
  }
};

const float moot = 8;

const float weapons_xpses[] = { -10 * moot, -10 * moot, 0 * moot, 0 * moot, 10 * moot, 10 * moot };
const float weapons_ypses[] = { 15 * moot, -15 * moot, 15 * moot, -5 * moot, 15 * moot, 5 * moot };

void ShopDemo::init(const IDBWeapon *weap, const Player *player) {
  StackString sst("Initting demo weapon shop");
  
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
  
  game.initDemo(&players, 20 * moot, weapons_xpses, weapons_ypses);
  
  int s = clock();
  //for(int i = 0; i < 60; i++)
    //runTick();              // make this faster somehow?
  int e = clock();
  dprintf("Did shopinfo preroll in %f seconds\n", float(e - s) / CLOCKS_PER_SEC);
};

int mult(int frams) {
  if(frams < 60)
    return 10;
  if(frams < 600)
    return 4;
  return 1;
}

void ShopDemo::runTick() {
  vector<GameAi *> tai;
  for(int i = 0; i < ais.size(); i++)
    tai.push_back(ais[i].get());
  for(int i = 0; i < mult(game.frameCount()); i++) {
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
  if(mult(game.frameCount()) != 1)
    drawJustifiedText(StringPrintf("%dx", mult(game.frameCount())), 0.1, 0, 1, TEXT_MIN, TEXT_MAX);
};

ShopDemo::ShopDemo() { };
ShopDemo::~ShopDemo() { };
