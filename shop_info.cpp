
#include "shop_info.h"

#include "gfx.h"
#include "debug.h"

ShopInfo::ShopInfo() { };

const float moot = 8;

const float xpses[] = { -10 * moot, -10 * moot, 0 * moot, 0 * moot, 10 * moot, 10 * moot };
const float ypses[] = { 15 * moot, -15 * moot, 15 * moot, -5 * moot, 15 * moot, 5 * moot };

void ShopInfo::init(const IDBWeapon *weap, const Player *player) {
  StackString sst("Initting demo weapon shop");
  
  players.clear();
  players.resize(6);
  CHECK(factionList().size() >= players.size());
  for(int i = 0; i < players.size(); i++) {
    players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode
    players[i].addCash(Money(1000000000));
    if(weap != defaultWeapon())
      for(int j = 0; j < 1000; j++)
        players[i].buyWeapon(weap);
  }
  
  game.initDemo(&players, 20 * moot, xpses, ypses);

};

void ShopInfo::runTick() {
  vector<Keystates> keese(6);
  for(int i = 0; i < 6; i++)
    if(i % 2 == 0)
      keese[i].fire[0].down = true;
  game.runTick(keese);
};

void ShopInfo::renderFrame() const {
  setZoom(0, 0, 1);
  setColor(1, 1, 1);
  game.renderToScreen();
};
