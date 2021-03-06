
#include "shop_info.h"

#include "gfx.h"
#include "parse.h"

using namespace std;

class ShopKVPrinter {
public:
  ShopKVPrinter(Float4 bounds, float fontsize, float linesize, float *writeend);
  ~ShopKVPrinter();
  
  void header(const string &val);
  void print(const string &key, const string &value);

  void discontinuity();

private:
  bool twolinemode() const;
  Float4 activebounds() const;

  vector<pair<string, string> > pairz;
  string head;

  Float4 bounds;
  float fontsize;
  float linesize;

  float *writeend;
};

ShopKVPrinter::ShopKVPrinter(Float4 in_bounds, float in_fontsize, float in_linesize, float *in_writeend) {
  bounds = in_bounds;
  fontsize = in_fontsize;
  linesize = in_linesize;
  writeend = in_writeend;
};

ShopKVPrinter::~ShopKVPrinter() {
  discontinuity();
};

void ShopKVPrinter::header(const string &in_head) {
  CHECK(!in_head.empty());
  CHECK(head.empty());
  head = in_head;
}

void ShopKVPrinter::print(const string &key, const string &value) {
  pairz.push_back(make_pair(key, value));
};

void ShopKVPrinter::discontinuity() {
  Float4 activerkt = activebounds();
  int step = twolinemode() + 1;
  int len = pairz.size() * step - 1;
  if(head.size())
    len += 2;
  setColor(C::inactive_text);
  Float4 tbx = Float4(activerkt.sx, activerkt.sy, activerkt.ex, activerkt.sy + len * linesize + fontsize);
  drawTextBoxAround(tbx, fontsize);
  for(int i = 0; i < pairz.size(); i++) {
    drawText(pairz[i].first, fontsize, Float2(activerkt.sx, activerkt.sy + linesize * (i + !!head.size() * 2) * step));
    
    if(pairz[i].second.size() > 4 && pairz[i].second.substr(pairz[i].second.size() - 4, 4) == " cme" && getTextWidth(pairz[i].second, fontsize) > activebounds().span_x())
      pairz[i].second = pairz[i].second.substr(0, pairz[i].second.size() - 4);
    drawJustifiedText(pairz[i].second, fontsize, Float2(activerkt.ex, activerkt.sy + linesize * (i * step + twolinemode() + !!head.size() * 2)), TEXT_MAX, TEXT_MIN);
  }
  if(head.size()) {
    setColor(C::active_text);
    drawJustifiedText(head, fontsize, Float2((activerkt.sx + activerkt.ex) / 2, activerkt.sy), TEXT_CENTER, TEXT_MIN);
  }
  pairz.clear();
  bounds.sy = tbx.ey + fontsize * 1.5;
  
  *writeend = bounds.sy;
}

bool ShopKVPrinter::twolinemode() const {
  float wid = activebounds().span_x();
  for(int i = 0; i < pairz.size(); i++)
    if(pairz[i].second.size() && getTextWidth(pairz[i].first, fontsize) + getTextWidth(pairz[i].second, fontsize) + getTextWidth("  ", fontsize) * !pairz[i].second.empty() > wid)
      return true;
  return false;
}

Float4 ShopKVPrinter::activebounds() const {
  return Float4(bounds.sx + fontsize / 3, bounds.sy + fontsize / 3, bounds.ex - fontsize / 3, bounds.ey - fontsize / 3);
}
  
ShopInfo::ShopInfo() {
  weapon = NULL;
}

void ShopInfo::null() {
  weapon = NULL;
  glory = NULL;
  bombardment = NULL;
  upgrade = NULL;
  tank = NULL;
  implant = NULL;
  implantslot = NULL;
}

void ShopInfo::init(const IDBWeapon *in_weapon, const Player *in_player, int in_playercount, bool equip_info, bool in_miniature) {
  null();
  miniature = in_miniature;
  weapon = in_weapon;
  text = in_weapon->launcher->text;
  playercount = in_playercount;
  weapon_equipinfo = equip_info;
  if(playercount == 1)
    playercount = 2;
  CHECK(playercount >= 2);
  if(!miniature)
    demo.init(weapon, in_player, NULL);
}
void ShopInfo::init(const IDBGlory *in_glory, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  glory = in_glory;
  text = in_glory->text;
  if(!miniature)
    demo.init(glory, in_player, NULL);
}
void ShopInfo::init(const IDBBombardment *in_bombardment, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  bombardment = in_bombardment;
  text = in_bombardment->text;
  if(!miniature)
    demo.init(bombardment, in_player, NULL);
}
void ShopInfo::init(const IDBUpgrade *in_upgrade, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  upgrade = in_upgrade;
  text = in_upgrade->text;
  // no working demo atm
}
void ShopInfo::init(const IDBTank *in_tank, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  tank = in_tank;
  text = in_tank->text;
  // no working demo atm
}
void ShopInfo::init(const IDBImplant *in_implant, bool upgrade, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  implant = in_implant;
  implant_upgrade = upgrade;
  text = in_implant->text;
  // no working demo atm
}
void ShopInfo::init(const IDBImplantSlot *in_implantslot, const Player *in_player, bool in_miniature) {
  null();
  miniature = in_miniature;
  implantslot = in_implantslot;
  text = in_implantslot->text;
  // no working demo atm
}

void ShopInfo::initIfNeeded(const IDBWeapon *in_weapon, const Player *in_player, int in_playercount, bool equip_info, bool in_miniature) {
  if(weapon != in_weapon || miniature != in_miniature)
    init(in_weapon, in_player, in_playercount, equip_info, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBGlory *in_glory, const Player *in_player, bool in_miniature) {
  if(glory != in_glory || miniature != in_miniature)
    init(in_glory, in_player, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBBombardment *in_bombardment, const Player *in_player, bool in_miniature) {
  if(bombardment != in_bombardment || miniature != in_miniature)
    init(in_bombardment, in_player, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBUpgrade *in_upgrade, const Player *in_player, bool in_miniature) {
  if(upgrade != in_upgrade || miniature != in_miniature)
    init(in_upgrade, in_player, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBTank *in_tank, const Player *in_player, bool in_miniature) {
  if(tank != in_tank || miniature != in_miniature)
    init(in_tank, in_player, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBImplant *in_implant, bool upgrade, const Player *in_player, bool in_miniature) {
  if(implant != in_implant || implant_upgrade != upgrade || miniature != in_miniature)
    init(in_implant, upgrade, in_player, in_miniature);
}
void ShopInfo::initIfNeeded(const IDBImplantSlot *in_implantslot, const Player *in_player, bool in_miniature) {
  if(implantslot != in_implantslot || miniature != in_miniature)
    init(in_implantslot, in_player, in_miniature);
}

void ShopInfo::clear() {
  null();
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
      int wordsa = wordsallowed(left, fontsize, rkt.span_x(), first ? "   " : "");
      CHECK(wordsa > 0 && wordsa <= left.size());
      string v = first ? "   " : "";
      for(int k = 0; k < wordsa; k++) {
        if(k)
          v += " ";
        v += left[k];
      }
      CHECK(getTextWidth(v, fontsize) <= rkt.span_x());
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

void ShopInfo::renderFrame(Float4 bounds, float fontsize, Float4 inset, const Player *player) const {
  StackString stp("ShopInfo");
  CHECK(bool(weapon) + bool(glory) + bool(bombardment) + bool(upgrade) + bool(tank) + bool(implant) + bool(implantslot) == 1);
  
  /*if(text && !miniature)
    drawShadedFormattedText(bounds, fontsize * 0.75, *text);
  
  bounds.sy += fontsize * 8;*/
  
  const float fontshift = fontsize * 1.5;
  float kvpend = 0;
  
  if(weapon) {
    StackString stp("ShopInfoWeapon");
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    kvp.print("Theoretical DPS", prettyFloatFormat(player->adjustWeapon(weapon).stats_damagePerSecond()));
    kvp.print("Cost/damage", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerDamage()));
    kvp.print("Cost/second", prettyFloatFormat(player->adjustWeapon(weapon).stats_costPerSecond()));
    if(player->adjustWeapon(weapon).glory_resistance()) {
      kvp.print("", "");
      kvp.print("Glory device resistance", "");
    }
    if(weapon->recommended != -1) {
      kvp.discontinuity();
      int recommended = int(sqrt((float)playercount) * player->adjustWeapon(weapon).recommended());
      kvp.header("Recommended");
      kvp.print("Loadout", StringPrintf("%d", recommended));
      kvp.print("Ammo packs", StringPrintf("%d", (int)ceil((float)recommended / weapon->quantity)));
      kvp.print("Cost", StringPrintf("%s", player->adjustWeapon(weapon).cost(recommended).textual(Money::TEXT_NORIGHTPAD | Money::TEXT_NOABBREV).c_str()));
    }
    
    if(weapon_equipinfo) {
      vector<string> et;
      et.push_back("Move over the weapon you want to equip, then push the weapon key you wish to assign it to.");
      drawFormattedTextBox(et, fontsize, Float4(inset.sx, inset.midpoint().y, inset.ex, inset.ey), C::active_text, C::box_border);
    }
  } else if(glory) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    kvp.print("Total average damage", prettyFloatFormat(player->adjustGlory(glory).stats_averageDamage()));
  } else if(bombardment) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    kvp.print("Damage per hit", prettyFloatFormat(player->adjustBombardment(bombardment, 0).stats_damagePerShot()));
    kvp.print("Firing delay", prettyFloatFormat(player->adjustBombardment(bombardment, 0).lockdelay()) + " seconds");
    kvp.print("Cooldown", prettyFloatFormat(player->adjustBombardment(bombardment, 0).unlockdelay()) + " second");
  } else if(upgrade) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    const Player unupg = getUnupgradedPlayer(player);
    const Player upg = getUpgradedPlayer(player);
    for(int i = 0; i < IDBAdjustment::LAST; i++) {
      if(upgrade->adjustment->adjustmentfactor(IDBAdjustment::IDBAType(i)) != 1.0) {
        kvp.print(adjust_human[i], formatChange(IDBAdjustment::IDBAType(i), unupg, upg, *upgrade->adjustment));
      }
    }
  } else if(tank) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    kvp.print("Max health", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).maxHealth()) + " cme");
    kvp.print("Turn speed", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).turnSpeed()) + " rad/s");
    kvp.print("Forward speed", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).maxSpeed()) + " m/s");
    kvp.print("Mass", prettyFloatFormat(player->adjustTankWithInstanceUpgrades(tank).mass()) + " tons");
    if(tank->upgrades.size())
      kvp.print("", "");
    for(int i = 0; i < tank->upgrades.size(); i++)
      kvp.print(tank->upgrades[i] + " upgrades", "");
    if(tank->adjustment) {
      kvp.print("", "");
      const IDBAdjustment *idba = tank->adjustment;
      for(int i = 0; i < ARRAY_SIZE(idba->adjustlist); i++) {
        if(idba->adjustlist[i].first == -1)
          break;
        
        kvp.print(adjust_human[idba->adjustlist[i].first], StringPrintf("%.0d%%", idba->adjustlist[i].second + 100));
      }
    }
  } else if(implant && !implant_upgrade) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    CHECK(implant->adjustment);
    const Player unimp = getUnimplantedPlayer(player);
    const Player imp = getImplantedPlayer(player);
    for(int i = 0; i < IDBAdjustment::LAST; i++) {
      if(implant->adjustment->adjustmentfactor(IDBAdjustment::IDBAType(i)) != 1.0) {
        kvp.print(adjust_human[i], formatChange(IDBAdjustment::IDBAType(i), unimp, imp, implant->makeAdjustment(player->implantLevel(implant))));
      }
    }
  } else if(implant && implant_upgrade) {
    ShopKVPrinter kvp(bounds, fontsize, fontshift, &kvpend);
    CHECK(implant->adjustment);
    const Player imp = getImplantedPlayer(player);
    const Player implev = getImplantedLeveledPlayer(player);
    for(int i = 0; i < IDBAdjustment::LAST; i++) {
      if(implant->adjustment->adjustmentfactor(IDBAdjustment::IDBAType(i)) != 1.0) {
        kvp.print(adjust_human[i], formatChange(IDBAdjustment::IDBAType(i), imp, implev, *implant->adjustment));
      }
    }
  } else if(implantslot) {
  } else {
    CHECK(0);
  }
  
  inset.sy = max(inset.sy, kvpend);
  
  if(hasDemo()) {
    GfxWindow gfxw(inset, 1.0);
    demo.renderFrame();
  }
}

string ShopInfo::formatChange(IDBAdjustment::IDBAType cat, const Player &before, const Player &after, const IDBAdjustment &adjust) {
  return StringPrintf("%s -> %s (+%.0f%%)%s", formatSlot(cat, before).c_str(), formatSlot(cat, after).c_str(), adjust.adjustmentfactor(cat) * 100 - 100, adjust_unit[cat]);
}

string ShopInfo::formatSlot(IDBAdjustment::IDBAType cat, const Player &player) {
  if(cat == IDBAdjustment::TANK_SPEED && player.hasValidTank()) {
    return prettyFloatFormat(player.getTank().maxSpeed());
  } else if(cat == IDBAdjustment::TANK_TURN && player.hasValidTank()) {
    return prettyFloatFormat(player.getTank().turnSpeed());
  } else if(cat == IDBAdjustment::TANK_ARMOR && player.hasValidTank()) {
    return prettyFloatFormat(player.getTank().maxHealth());
  } else {
    // fallback
    return StringPrintf("%.0f%%", player.getAdjust().adjustmentfactor(cat) * 100);
  }
}  
Player ShopInfo::getUnupgradedPlayer(const Player *player) const {
  CHECK(upgrade);
  Player ploy = *player;
  if(ploy.hasUpgrade(upgrade))
    ploy.forceRemoveUpgrade(upgrade);
  return ploy;
}

Player ShopInfo::getUpgradedPlayer(const Player *player) const {
  CHECK(upgrade);
  Player ploy = *player;
  if(!ploy.hasUpgrade(upgrade))
    ploy.forceAcquireUpgrade(upgrade);
  return ploy;
}

Player ShopInfo::getUnimplantedPlayer(const Player *player) const {
  CHECK(implant);
  Player ploy = *player;
  if(ploy.hasImplant(implant))
    ploy.toggleImplant(implant);
  return ploy;
}

Player ShopInfo::getImplantedPlayer(const Player *player) const {
  CHECK(implant);
  Player ploy = *player;
  if(!ploy.hasImplant(implant))
    ploy.forceAcquireImplant(implant);
  return ploy;
}

Player ShopInfo::getImplantedLeveledPlayer(const Player *player) const {
  CHECK(implant);
  Player ploy = *player;
  if(!ploy.hasImplant(implant))
    ploy.forceAcquireImplant(implant);
  ploy.forceLevelImplant(implant);
  return ploy;
}

bool ShopInfo::hasDemo() const {
  return (weapon || glory || bombardment) && !miniature && !(weapon && weapon_equipinfo);
}

