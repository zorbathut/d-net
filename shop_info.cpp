
#include "shop_info.h"
#include "gfx.h"

ShopInfo::ShopInfo() {
  weapon = NULL;
  player = NULL;
}

void ShopInfo::init(const IDBWeapon *in_weapon, const Player *in_player) {
  weapon = in_weapon;
  player = in_player;
  demo.init(weapon, player);
}
  
void ShopInfo::runTick() {
  demo.runTick();
}
  
void ShopInfo::renderFrame(Float4 bounds, float fontsize, Float4 inset) const {
  const float fontshift = fontsize * 1.5;
  int lineid = 0;
  drawText("theoretical dps", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_damagePerSecond()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  drawText("cost per damage", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_costPerDamage()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  drawText("cost per second", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  drawText(StringPrintf("%20.4f", player->adjustWeapon(weapon).stats_costPerSecond()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  
  GfxWindow gfxw(inset);
  demo.renderFrame();
}
