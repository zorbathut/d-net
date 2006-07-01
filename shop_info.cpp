
#include "shop_info.h"
#include "gfx.h"

ShopInfo::ShopInfo() {
  weapon = NULL;
  player = NULL;
}

void ShopInfo::null() {
  weapon = NULL;
  glory = NULL;
  bombardment = NULL;
  upgrade = NULL;
  tank = NULL;
  player = NULL;
}

void ShopInfo::init(const IDBWeapon *in_weapon, const Player *in_player) {
  null();
  weapon = in_weapon;
  player = in_player;
  demo.init(weapon, player);
}
void ShopInfo::init(const IDBGlory *in_glory, const Player *in_player) {
  null();
  glory = in_glory;
  player = in_player;
  // no working demo atm
}
void ShopInfo::init(const IDBBombardment *in_bombardment, const Player *in_player) {
  null();
  bombardment = in_bombardment;
  player = in_player;
  // no working demo atm
}
void ShopInfo::init(const IDBUpgrade *in_upgrade, const Player *in_player) {
  null();
  upgrade = in_upgrade;
  player = in_player;
  // no working demo atm
}
void ShopInfo::init(const IDBTank *in_tank, const Player *in_player) {
  null();
  tank = in_tank;
  player = in_player;
  // no working demo atm
}
  
void ShopInfo::runTick() {
  if(weapon)
    demo.runTick();
}
  
void ShopInfo::renderFrame(Float4 bounds, float fontsize, Float4 inset) const {
  CHECK(bool(weapon) + bool(glory) + bool(bombardment) + bool(upgrade) + bool(tank) == 1);
  
  const float fontshift = fontsize * 1.5;
  if(weapon) {
    int lineid = 0;
    
    drawText("theoretical dps", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_damagePerSecond()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("cost per damage", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_costPerDamage()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("cost per second", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_costPerSecond()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    
    GfxWindow gfxw(inset);
    demo.renderFrame();
  } else if(glory) {
    int lineid = 0;
    
    drawText("total average damage", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%20.4f", player->adjustGlory(glory).stats_averageDamage()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  } else if(bombardment) {
    int lineid = 0;
    
    drawText("damage per hit", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%20.4f", player->adjustBombardment(bombardment).warhead().stats_damagePerShot()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("firing delay", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%12.0f seconds", (float)player->adjustBombardment(bombardment).lockdelay() / FPS), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("cooldown", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%12.0f seconds", (float)player->adjustBombardment(bombardment).unlockdelay() / FPS), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  } else {
    drawText("unintted", fontsize, bounds.sx, bounds.sy);
    //CHECK(0);
  }
}
