
#include "shop_info.h"
#include "gfx.h"

class ShopKVPrinter {
public:
  ShopKVPrinter(Float4 bounds, float fontsize, float linesize);
  ~ShopKVPrinter();
  
  void print(const string &key, const string &value);

private:
  vector<pair<string, string> > pairz;

  Float4 bounds;
  float fontsize;
  float linesize;
};

ShopKVPrinter::ShopKVPrinter(Float4 in_bounds, float in_fontsize, float in_linesize) {
  bounds = in_bounds;
  fontsize = in_fontsize;
  linesize = in_linesize;
};

ShopKVPrinter::~ShopKVPrinter() {
  Float4 activerkt(bounds.sx + fontsize / 3, bounds.sy + fontsize / 3, bounds.ex - fontsize / 3, bounds.ey - fontsize / 3);
  Float4 rkt(activerkt.sx - fontsize / 3, activerkt.sy - fontsize / 3, activerkt.ex + fontsize / 3, activerkt.sy + (pairz.size() - 1) * linesize + fontsize + fontsize / 3);
  drawSolid(rkt);
  setColor(0.3, 0.3, 0.3);
  drawRect(rkt, 0.1);
  setColor(1.0, 1.0, 1.0);
  for(int i = 0; i < pairz.size(); i++) {
    drawText(pairz[i].first, fontsize, activerkt.sx, activerkt.sy + linesize * i);
    drawJustifiedText(pairz[i].second, fontsize, activerkt.ex, activerkt.sy + linesize * i, TEXT_MAX, TEXT_MIN);
  }
};

void ShopKVPrinter::print(const string &key, const string &value) {
  pairz.push_back(make_pair(key, value));
};
  
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
    {
      ShopKVPrinter kvp(bounds, fontsize, fontshift);
      kvp.print("theoretical dps", StringPrintf(adjust_format[IDBAdjustment::DAMAGE_KINETIC], player->adjustWeapon(weapon).stats_damagePerSecond()));
      kvp.print("cost per damage", StringPrintf("%.2f", player->adjustWeapon(weapon).stats_costPerDamage()));
      kvp.print("cost per second", StringPrintf("%.2f", player->adjustWeapon(weapon).stats_costPerSecond()));
    }
    
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
    drawText(StringPrintf("%12.0f seconds", player->adjustBombardment(bombardment).lockdelay()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("cooldown", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%12.0f seconds", player->adjustBombardment(bombardment).unlockdelay()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  } else if(upgrade) {
    int lineid = 0;
    
    for(int i = 0; i < IDBAdjustment::LAST; i++) {
      if(upgrade->adjustment->adjustmentfactor(i) != 1.0) {
        // %s -> %s (%4.2f%%) %s
        drawText(adjust_human[i], fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
        //bool adjmult = !player->hasUpgrade(upgrade);
        drawText(StringPrintf("%s -> %s (%4.2f%%)%s", getUpgradeBefore(i).c_str(), getUpgradeAfter(i).c_str(), upgrade->adjustment->adjustmentfactor(i) * 100 - 100, adjust_unit[i]), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
      }
    }
  } else if(tank) {
    int lineid = 0;
    
    drawText("max health", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%12.0fcm equiv", player->adjustTank(tank).maxHealth()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("turn speed", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%14.1f rad/s", player->adjustTank(tank).turnSpeed()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText("forward speed", fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
    drawText(StringPrintf("%16.0f m/s", player->adjustTank(tank).maxSpeed()), fontsize, bounds.sx, bounds.sy + fontshift * lineid++);
  } else {
    drawText("unintted", fontsize, bounds.sx, bounds.sy);
    //CHECK(0);
  }
}

string ShopInfo::getUpgradeBefore(int cat) const {
  if(cat == IDBAdjustment::TANK_SPEED) {
    Player tplayer = getUnupgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_SPEED], tplayer.getTank().maxSpeed());
  } else if(cat == IDBAdjustment::TANK_TURN) {
    Player tplayer = getUnupgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_TURN], tplayer.getTank().turnSpeed());
  } else if(cat == IDBAdjustment::TANK_ARMOR) {
    Player tplayer = getUnupgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_ARMOR], tplayer.getTank().maxHealth());
  } else {
    // fallback
    return StringPrintf("%4f.0", player->getAdjust().adjustmentfactor(cat) * 100);
  }
}

string ShopInfo::getUpgradeAfter(int cat) const {
  if(cat == IDBAdjustment::TANK_SPEED) {
    Player tplayer = getUpgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_SPEED], tplayer.getTank().maxSpeed());
  } else if(cat == IDBAdjustment::TANK_TURN) {
    Player tplayer = getUpgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_TURN], tplayer.getTank().turnSpeed());
  } else if(cat == IDBAdjustment::TANK_ARMOR) {
    Player tplayer = getUpgradedPlayer();
    return StringPrintf(adjust_format[IDBAdjustment::TANK_ARMOR], tplayer.getTank().maxHealth());
  } else {
    // fallback
    return StringPrintf("%4f.0", (player->getAdjust().adjustmentfactor(cat) + upgrade->adjustment->adjustmentfactor(cat)) * 100 - 100);
  }
}

Player ShopInfo::getUnupgradedPlayer() const {
  Player ploy = *player;
  if(ploy.hasUpgrade(upgrade))
    ploy.forceRemoveUpgrade(upgrade);
  return ploy;
}

Player ShopInfo::getUpgradedPlayer() const {
  Player ploy = *player;
  if(!ploy.hasUpgrade(upgrade))
    ploy.forceAcquireUpgrade(upgrade);
  return ploy;
}

