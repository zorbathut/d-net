
#include "shop_info.h"
#include "gfx.h"

class ShopKVPrinter {
public:
  ShopKVPrinter(Float4 bounds, float fontsize, float linesize);
  ~ShopKVPrinter();
  
  void print(const string &key, const string &value);

private:
  bool twolinemode() const;
  Float4 activebounds() const;

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
  Float4 activerkt = activebounds();
  int step = twolinemode() + 1;
  Float4 rkt(activerkt.sx - fontsize / 3, activerkt.sy - fontsize / 3, activerkt.ex + fontsize / 3, activerkt.sy + (pairz.size() * step - 1) * linesize + fontsize + fontsize / 3);
  drawSolid(rkt);
  setColor(0.3, 0.3, 0.3);
  drawRect(rkt, 0.1);
  setColor(1.0, 1.0, 1.0);
  for(int i = 0; i < pairz.size(); i++) {
    drawText(pairz[i].first, fontsize, activerkt.sx, activerkt.sy + linesize * i * step);
    drawJustifiedText(pairz[i].second, fontsize, activerkt.ex, activerkt.sy + linesize * (i * step + twolinemode()), TEXT_MAX, TEXT_MIN);
  }
};

void ShopKVPrinter::print(const string &key, const string &value) {
  pairz.push_back(make_pair(key, value));
};

bool ShopKVPrinter::twolinemode() const {
  float wid = activebounds().x_span();
  for(int i = 0; i < pairz.size(); i++)
    if(getTextWidth(pairz[i].first, fontsize) + getTextWidth(pairz[i].second, fontsize) + getTextWidth("  ", fontsize) > wid)
      return true;
  return false;
}

Float4 ShopKVPrinter::activebounds() const {
  return Float4(bounds.sx + fontsize / 3, bounds.sy + fontsize / 3, bounds.ex - fontsize / 3, bounds.ey - fontsize / 3);
}
  
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
  //demo.init(bombardment, player);
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
      kvp.print("theoretical dps", prettyFloatFormat(player->adjustWeapon(weapon).stats_damagePerSecond()));
      kvp.print("cost per damage", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerDamage()));
      kvp.print("cost per second", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerSecond()));
    }
    
    GfxWindow gfxw(inset);
    demo.renderFrame();
  } else if(glory) {
    {
      ShopKVPrinter kvp(bounds, fontsize, fontshift);
      
      kvp.print("total average damage", prettyFloatFormat(player->adjustGlory(glory).stats_averageDamage()));
    }
  } else if(bombardment) {
    {
      ShopKVPrinter kvp(bounds, fontsize, fontshift);
      kvp.print("damage per hit", prettyFloatFormat(player->adjustBombardment(bombardment).warhead().stats_damagePerShot()));
      kvp.print("firing delay", prettyFloatFormat(player->adjustBombardment(bombardment).lockdelay()) + " seconds");
      kvp.print("cooldown", prettyFloatFormat(player->adjustBombardment(bombardment).unlockdelay()) + " second");
    }
  } else if(upgrade) {
    {
      ShopKVPrinter kvp(bounds, fontsize, fontshift);
      
      for(int i = 0; i < IDBAdjustment::LAST; i++) {
        if(upgrade->adjustment->adjustmentfactor(i) != 1.0) {
          kvp.print(adjust_human[i], StringPrintf("%s -> %s (%s%%)%s", getUpgradeBefore(i).c_str(), getUpgradeAfter(i).c_str(), prettyFloatFormat(upgrade->adjustment->adjustmentfactor(i) * 100 - 100).c_str(), adjust_unit[i]));
        }
      }
    }
  } else if(tank) {
    {
      ShopKVPrinter kvp(bounds, fontsize, fontshift);
      
      kvp.print("max health", prettyFloatFormat(player->adjustTank(tank).maxHealth()) + " cme");
      kvp.print("turn speed", prettyFloatFormat(player->adjustTank(tank).turnSpeed()) + " rad/s");
      kvp.print("forward speed", prettyFloatFormat(player->adjustTank(tank).maxSpeed()) + " m/s");
      kvp.print("mass", prettyFloatFormat(player->adjustTank(tank).mass()) + " tons");
    }
  } else {
    CHECK(0);
  }
}

string ShopInfo::getUpgradeBefore(int cat) const {
  if(cat == IDBAdjustment::TANK_SPEED) {
    Player tplayer = getUnupgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().maxSpeed());
  } else if(cat == IDBAdjustment::TANK_TURN) {
    Player tplayer = getUnupgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().turnSpeed());
  } else if(cat == IDBAdjustment::TANK_ARMOR) {
    Player tplayer = getUnupgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().maxHealth());
  } else {
    // fallback
    return prettyFloatFormat(player->getAdjust().adjustmentfactor(cat) * 100);
  }
}

string ShopInfo::getUpgradeAfter(int cat) const {
  if(cat == IDBAdjustment::TANK_SPEED) {
    Player tplayer = getUpgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().maxSpeed());
  } else if(cat == IDBAdjustment::TANK_TURN) {
    Player tplayer = getUpgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().turnSpeed());
  } else if(cat == IDBAdjustment::TANK_ARMOR) {
    Player tplayer = getUpgradedPlayer();
    return prettyFloatFormat(tplayer.getTank().maxHealth());
  } else {
    // fallback
    return prettyFloatFormat((player->getAdjust().adjustmentfactor(cat) + upgrade->adjustment->adjustmentfactor(cat)) * 100 - 100);
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

