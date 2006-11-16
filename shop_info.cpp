
#include "shop_info.h"

#include "gfx.h"
#include "parse.h"
#include "player.h"

using namespace std;

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
  setColor(C::inactive_text);
  drawTextBoxAround(Float4(activerkt.sx, activerkt.sy, activerkt.ex, activerkt.sy + (pairz.size() * step - 1) * linesize + fontsize), fontsize);
  for(int i = 0; i < pairz.size(); i++) {
    drawText(pairz[i].first, fontsize, Float2(activerkt.sx, activerkt.sy + linesize * i * step));
    drawJustifiedText(pairz[i].second, fontsize, Float2(activerkt.ex, activerkt.sy + linesize * (i * step + twolinemode())), TEXT_MAX, TEXT_MIN);
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

void ShopInfo::init(const IDBWeapon *in_weapon, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  weapon = in_weapon;
  player = in_player;
  text = in_weapon->launcher->text;
  if(!miniature)
    demo.init(weapon, player, NULL);
}
void ShopInfo::init(const IDBGlory *in_glory, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  glory = in_glory;
  player = in_player;
  text = in_glory->text;
  if(!miniature)
    demo.init(glory, player, NULL);
}
void ShopInfo::init(const IDBBombardment *in_bombardment, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  bombardment = in_bombardment;
  player = in_player;
  text = in_bombardment->text;
  if(!miniature)
    demo.init(bombardment, player, NULL);
}
void ShopInfo::init(const IDBUpgrade *in_upgrade, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  upgrade = in_upgrade;
  player = in_player; 
  text = in_upgrade->text;
  // no working demo atm
}
void ShopInfo::init(const IDBTank *in_tank, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  tank = in_tank;
  player = in_player;
  text = in_tank->text;
  // no working demo atm
}
  
void ShopInfo::runTick() {
  if(hasDemo())
    demo.runTick();
}

int wordsallowed(const vector<string> &left, float fontsize, float limit, const string &start) {
  for(int i = 0; i <= left.size(); i++) {
    string v = start;
    for(int k = 0; k < i; k++) {
      if(k)
        v += " ";
      v += left[k];
    }
    if(getTextWidth(v, fontsize) > limit)
      return i - 1;
  }
  return left.size();
}

void drawShadedFormattedText(Float4 bounds, float fontsize, const string &text) {
  float linesize = fontsize * 1.5;
  Float4 rkt(bounds.sx + fontsize / 3, bounds.sy + fontsize / 3, bounds.ex - fontsize / 3, bounds.sy + fontsize / 3);
    
  vector<string> lines = tokenize(text, "\n");
  vector<string> vlines;
  for(int i = 0; i < lines.size(); i++) {
    vector<string> left = tokenize(lines[i], " ");
    bool first = true;
    while(left.size()) {
      int wordsa = wordsallowed(left, fontsize, rkt.x_span(), first ? "   " : "");
      CHECK(wordsa > 0 && wordsa <= left.size());
      string v = first ? "   " : "";
      for(int k = 0; k < wordsa; k++) {
        if(k)
          v += " ";
        v += left[k];
      }
      CHECK(getTextWidth(v, fontsize) <= rkt.x_span());
      vlines.push_back(v);
      first = false;
      left.erase(left.begin(), left.begin() + wordsa);
    }
  }
  
  rkt.ey = rkt.sy + (vlines.size() - 1) * linesize + fontsize;
  
  drawTextBoxAround(rkt, fontsize);
  
  setColor(C::inactive_text);
  for(int i = 0; i < vlines.size(); i++)
    drawText(vlines[i], fontsize, Float2(rkt.sx, rkt.sy + linesize * i));
}

void ShopInfo::renderFrame(Float4 bounds, float fontsize, Float4 inset) const {
  CHECK(bool(weapon) + bool(glory) + bool(bombardment) + bool(upgrade) + bool(tank) == 1);
  
  if(text && !miniature)
    drawShadedFormattedText(bounds, fontsize, *text);
  
  bounds.sy += 25;
  
  const float fontshift = fontsize * 1.5;
  if(weapon) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift);
    kvp.print("Theoretical DPS", prettyFloatFormat(player->adjustWeapon(weapon).stats_damagePerSecond()));
    kvp.print("Cost per damage", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerDamage()));
    kvp.print("Cost per second", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerSecond()));
  } else if(glory) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift);
    kvp.print("Total average damage", prettyFloatFormat(player->adjustGlory(glory).stats_averageDamage()));
  } else if(bombardment) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift);
    kvp.print("Damage per hit", prettyFloatFormat(player->adjustBombardment(bombardment, 0).warhead().stats_damagePerShot()));
    kvp.print("Firing delay", prettyFloatFormat(player->adjustBombardment(bombardment, 0).lockdelay()) + " seconds");
    kvp.print("Cooldown", prettyFloatFormat(player->adjustBombardment(bombardment, 0).unlockdelay()) + " second");
  } else if(upgrade) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift);
    for(int i = 0; i < IDBAdjustment::LAST; i++) {
      if(upgrade->adjustment->adjustmentfactor(i) != 1.0) {
        if(i >= 0 && i < IDBAdjustment::DAMAGE_LAST) {
          kvp.print(adjust_human[i], StringPrintf("%s -> %s (%.0f%%)%s", getUpgradeBefore(i).c_str(), getUpgradeAfter(i).c_str(), upgrade->adjustment->adjustmentfactor(i) * 100 - 100, adjust_unit[i]));
        } else {
          kvp.print(adjust_human[i], StringPrintf("%s -> %s (%s%%)%s", getUpgradeBefore(i).c_str(), getUpgradeAfter(i).c_str(), prettyFloatFormat(upgrade->adjustment->adjustmentfactor(i) * 100 - 100).c_str(), adjust_unit[i]));
        }
      }
    }
  } else if(tank) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift);
    kvp.print("Max health", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).maxHealth()) + " cme");
    kvp.print("Turn speed", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).turnSpeed()) + " rad/s");
    kvp.print("Forward speed", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).maxSpeed()) + " m/s");
    kvp.print("Mass", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).mass()) + " tons");
    for(int i = 0; i < tank->upgrades.size(); i++)
      kvp.print(tank->upgrades[i] + " upgrades", "");
    if(tank->adjustment) {
      for(int i = 0; i < IDBAdjustment::LAST; i++) {
        if(tank->adjustment->adjustmentfactor(i) != 1.0) {
          kvp.print(adjust_human[i], StringPrintf("%.0f%%", tank->adjustment->adjustmentfactor(i) * 100));
        }
      }
    }
  } else {
    CHECK(0);
  }
  
  if(hasDemo()) {
    GfxWindow gfxw(inset, 1.0);
    demo.renderFrame();
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
  } else if(cat < IDBAdjustment::DAMAGE_LAST) {
    return StringPrintf("%.0f%%", player->getAdjust().adjustmentfactor(cat) * 100);
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
  } else if(cat < IDBAdjustment::DAMAGE_LAST) {
    return StringPrintf("%.0f%%", (player->getAdjust().adjustmentfactor(cat) + upgrade->adjustment->adjustmentfactor(cat)) * 100 - 100);
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

bool ShopInfo::hasDemo() const {
  return (weapon || glory || bombardment) && !miniature;
}

