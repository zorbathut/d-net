
#include "shop_info.h"

#include "gfx.h"

ShopInfo::ShopInfo() { };

const float xpses[] = { -20, -20, 0, 0, 20, 20 };
const float ypses[] = { 10, -20, 10, -10, 10, 0 };

void ShopInfo::init(const IDBWeapon *weap, const Player *player) {
  players.clear();
  players.resize(6);
  CHECK(factionList().size() >= players.size());
  for(int i = 0; i < players.size(); i++) {
    players[i] = Player(&factionList()[i], 0); // TODO: make this be the right faction mode
  }
  
  
    
};

void ShopInfo::runTick() { }; // wheeeeeee
void ShopInfo::renderFrame() const {
  setZoom(0, 0, 1);
  setColor(1, 1, 1);
  drawRect(Float4(0, 0, 1, 1), 0.1);
};
