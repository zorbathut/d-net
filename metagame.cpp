
#include "metagame.h"
#include "gfx.h"
#include "parse.h"
#include "rng.h"
#include "inputsnag.h"
#include "args.h"

#include <string>
#include <numeric>
#include <fstream>

using namespace std;

DEFINE_int(factionMode, -1, "Faction mode to skip faction choice battle");

const HierarchyNode &Shop::getStepNode(int step) const {
  CHECK(step >= 0 && step <= curloc.size());
  const HierarchyNode *cnode = &itemDbRoot();
  for(int i = 0; i < step; i++) {
    CHECK(curloc[i] >= 0 && curloc[i] < cnode->branches.size());
    cnode = &cnode->branches[curloc[i]];
  }
  return *cnode;
}

const HierarchyNode &Shop::getCurNode() const {
  return getStepNode(curloc.size());
}

const HierarchyNode &Shop::getCategoryNode() const {
  return getStepNode(curloc.size() - 1);
}

const float sl_totalwidth = 133.334;

const float sl_hoffset = 1.5;
const float sl_voffset = 5;

const float sl_fontsize = 2;
const float sl_boxborder = 0.5;
const float sl_itemheight = 4;

const float sl_boxwidth = (sl_totalwidth - sl_hoffset * 3) / 2;

const float sl_pricehpos = sl_boxwidth / 4 * 3;

const float sl_boxthick = 0.1;

void Shop::renderNode(const HierarchyNode &node, int depth) const {
  float hoffbase = sl_hoffset + (sl_boxwidth + sl_hoffset) * (depth - xofs);
  
  for(int i = 0; i < node.branches.size(); i++) {
    if(depth < curloc.size() && curloc[depth] == i) {
      setColor(1.0, 1.0, 1.0);
      renderNode(node.branches[i], depth + 1);
    } else {
      setColor(0.3, 0.3, 0.3);
    }
    {
      float xstart = hoffbase;
      float xend = hoffbase + sl_boxwidth;
      xend = min(xend, 133.334f);
      xstart = max(xstart, 0.f);
      if(xend - xstart < sl_boxwidth * 0.01)
        continue;   // if we can only see 1% of this box, just don't show any of it - gets rid of some ugly rendering edge cases
    }
    drawSolid(Float4( hoffbase, sl_voffset + i * sl_itemheight, hoffbase + sl_boxwidth, sl_voffset + i * sl_itemheight + sl_fontsize + sl_boxborder * 2 ));
    drawRect(Float4( hoffbase, sl_voffset + i * sl_itemheight, hoffbase + sl_boxwidth, sl_voffset + i * sl_itemheight + sl_fontsize + sl_boxborder * 2 ), sl_boxthick);
    setColor(1.0, 1.0, 1.0);
    drawText(node.branches[i].name.c_str(), sl_fontsize, hoffbase + sl_boxborder, sl_voffset + i * sl_itemheight + sl_boxborder);
    {
      int dispmode = node.branches[i].displaymode;
      if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
        if(node.branches[i].type == HierarchyNode::HNT_UPGRADE && !player->hasUpgrade(node.branches[i].upgrade))
          dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[i].type == HierarchyNode::HNT_GLORY && !player->hasGlory(node.branches[i].glory))
          dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[i].type == HierarchyNode::HNT_BOMBARDMENT && !player->hasBombardment(node.branches[i].bombardment))
          dispmode = HierarchyNode::HNDM_COST;
      }
      if(dispmode == HierarchyNode::HNDM_BLANK) {
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        drawText(StringPrintf("%6s", node.branches[i].cost(player).textual().c_str()), sl_fontsize, hoffbase + sl_pricehpos, sl_voffset + i * sl_itemheight + sl_boxborder);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawText(StringPrintf("%dpk", node.branches[i].pack), sl_fontsize, hoffbase + sl_pricehpos, sl_voffset + i * sl_itemheight + sl_boxborder);
      } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
        drawText("bought", sl_fontsize, hoffbase + sl_pricehpos, sl_voffset + i * sl_itemheight + sl_boxborder);
      } else {
        CHECK(0);
      }
    }
  }
}

bool Shop::runTick(const Keystates &keys) {
  if(keys.l.repeat && curloc.size() > 1)
    curloc.pop_back();
  if(keys.r.repeat && getCurNode().branches.size() != 0)
    curloc.push_back(0);
  if(keys.u.repeat)
    curloc.back()--;
  if(keys.d.repeat)
    curloc.back()++;
  curloc.back() += getCategoryNode().branches.size();
  curloc.back() %= getCategoryNode().branches.size();
  
  if(keys.fire.repeat && getCurNode().buyable) {
    // Player is trying to buy something!
    
    // First check to see if we're allowed to buy it
    bool canBuy = false;
    if(getCurNode().type == HierarchyNode::HNT_DONE) {
      canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
      if(player->canBuyUpgrade(getCurNode().upgrade))
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
      if(player->canBuyWeapon(getCurNode().weapon))
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
      if(player->canBuyGlory(getCurNode().glory))
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
      if(player->canBuyBombardment(getCurNode().bombardment))
        canBuy = true;
    } else {
      CHECK(0);
    }
    
    // If so, buy it
    if(canBuy) {
      if(getCurNode().type == HierarchyNode::HNT_DONE) {
        return true;
      } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
        player->buyUpgrade(getCurNode().upgrade);
      } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
        player->buyWeapon(getCurNode().weapon);
      } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
        player->buyGlory(getCurNode().glory);
      } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
        player->buyBombardment(getCurNode().bombardment);
      } else {
        CHECK(0);
      }
    }
    
  }
  
  doTableUpdate();
  
  return false;
}

void Shop::ai(Ai *ais) const {
  if(ais)
    ais->updateShop(player);
}

const float framechange = 0.2;

void Shop::doTableUpdate() {
  float nxofs = max((int)curloc.size() - 1 - !getCurNode().branches.size(), 0);
  if(abs(xofs - nxofs) <= framechange)
    xofs = nxofs;
  else if(xofs < nxofs)
    xofs += framechange;
  else if(xofs > nxofs)
    xofs -= framechange;
  else
    CHECK(0);  // oh god bear is driving car
};

void Shop::doTableRender() const {
  renderNode(itemDbRoot(), 0);
};

void Shop::renderToScreen() const {
  CHECK(player);
  clearFrame(player->getFaction()->color * 0.05 + Color(0.05, 0.05, 0.05));
  setColor(1.0, 1.0, 1.0);
  setZoom(0, 0, 100);
  drawText(StringPrintf("cash onhand %s", player->getCash().textual().c_str()), 2, 80, 1);
  {
    drawText("there's like guns and shit here", 2, 1, 1);
  }
  setColor(player->getFaction()->color * 0.5);
  {
    const float ofs = 8;
    drawDvec2(player->getFaction()->icon, Float4(ofs, ofs, 125 - ofs, 100 - ofs), 50, 0.5);
  }
  doTableRender();
  float hudstart = itemDbRoot().branches.size() * sl_itemheight + sl_voffset + sl_boxborder;
  if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
    drawText("damage per second", 2, 1.5, hudstart);
    drawText(StringPrintf("%20.4f", player->adjustWeapon(getCurNode().weapon).stats_damagePerSecond()), 2, 1.5, hudstart + 3);
    drawText("cost per damage", 2, 1.5, hudstart + 6);
    drawText(StringPrintf("%20.4f", player->adjustWeapon(getCurNode().weapon).stats_costPerDamage()), 2, 1.5, hudstart + 9);
  } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
    drawText("total average damage", 2, 1.5, hudstart);
    drawText(StringPrintf("%20.4f", player->adjustGlory(getCurNode().glory).stats_averageDamage()), 2, 1.5, hudstart + 3);
  }
}

// Not a valid state
Shop::Shop() {
  player = NULL;
}

Shop::Shop(Player *in_player) {
  player = in_player;
  curloc.push_back(0);
  xofs = 0;
}

PlayerMenuState::PlayerMenuState() {
  settingmode = -12345;
  choicemode = -12345;

  setting_button_current = -1123;
  setting_button_reading = false;
  
  setting_axis_current = -1123;
  setting_axis_reading = false;
  
  setting_axistype = 1237539;
  
  test_game = NULL;
  test_player = NULL;
  
  memset(buttons, -1, sizeof(buttons));
  memset(axes, -1, sizeof(axes));
  
  faction = NULL;
  compasspos = Float2(0,0);
}

PlayerMenuState::PlayerMenuState(Float2 cent) {
  settingmode = SETTING_BUTTONS;
  choicemode = CHOICE_FIRSTPASS;
  
  setting_button_current = -1;
  setting_button_reading = true;
  
  setting_axis_current = -1;
  setting_axis_reading = true;
  
  setting_axistype = 0;
  
  test_game = NULL;
  test_player = NULL;
  
  memset(buttons, -1, sizeof(buttons));
  memset(axes, -1, sizeof(axes));
  
  faction = NULL;
  compasspos = cent;
}

bool PlayerMenuState::readyToPlay() const {
  return faction && settingmode == SETTING_READY && fireHeld == 60;
}

void PlayerMenuState::traverse_axistype(int delta, int axes) {
  do {
    setting_axistype += delta;
    setting_axistype += KSAX_END;
    setting_axistype %= KSAX_END;
  } while(ksax_minaxis[setting_axistype] > axes);
}

Keystates genKeystate(const Controller &keys, const PlayerMenuState &pms) {
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  kst.fire = keys.keys[pms.buttons[BUTTON_ACCEPT]];
  kst.change = keys.keys[pms.buttons[BUTTON_CANCEL]];
  kst.axmode = pms.setting_axistype;
  for(int j = 0; j < 2; j++) {
    kst.ax[j] = keys.axes[pms.axes[j]];
    if(pms.axes_invert[j])
      kst.ax[j] *= -1;
  }
  kst.udlrax[0] = keys.menu.x;
  kst.udlrax[1] = keys.menu.y;
  CHECK(keys.menu.x >= -1 && keys.menu.x <= 1);
  CHECK(keys.menu.y >= -1 && keys.menu.y <= 1);
  return kst;
}

vector<Keystates> genKeystates(const vector<Controller> &keys, const vector<PlayerMenuState> &modes) {
  vector<Keystates> kst;
  for(int i = 0; i < modes.size(); i++)
    if(modes[i].faction)
      kst.push_back(genKeystate(keys[i], modes[i]));
  return kst;
}

void standardButtonTick(int *outkeys, bool *outinvert, int outkeycount, int *current_button, bool *current_mode, const Controller &keys, const vector<float> &triggers, PlayerMenuState *pms) {
  if(*current_button == -1) {
    *current_button = 0;
    *current_mode = (pms->choicemode == CHOICE_FIRSTPASS);
  }
  CHECK(*current_button >= 0 && *current_button < outkeycount);
  if(*current_mode) {
    CHECK(pms->choicemode != CHOICE_IDLE);
    for(int i = 0; i < triggers.size(); i++) {
      if(abs(triggers[i]) > 0.9) {
        int j;
        bool noopify = false;
        for(j = 0; j < outkeycount; j++) {
          if(outkeys[j] == i) {
            if(outkeys[*current_button] != -1) {
              swap(outkeys[j], outkeys[*current_button]);
              if(outinvert) {
                swap(outinvert[j], outinvert[*current_button]);
                outinvert[*current_button] = (triggers[i] < 0);
              }
            } else {
              noopify = true;
            }
            break;
          }
        }
        if(noopify)
          continue;
        if(j == BUTTON_LAST) {
          outkeys[*current_button] = i;
          if(outinvert)
            outinvert[*current_button] = (triggers[i] < 0);
        }
        if(pms->choicemode == CHOICE_FIRSTPASS) {
          (*current_button)++;
        } else {
          *current_mode = false;
        }
        break;
      }
    }
    if(*current_button == outkeycount) {
      // Done with the first pass here - this can only happen if the choice if FIRSTPASS
      CHECK(pms->choicemode == CHOICE_FIRSTPASS);
      pms->settingmode++;
      (*current_button) = -1;
    }
  } else {
    CHECK(pms->choicemode == CHOICE_ACTIVE);
    if(keys.u.repeat)
      (*current_button)--;
    if(keys.d.repeat)
      (*current_button)++;
    if(*current_button >= outkeycount)
      *current_button = outkeycount - 1;
    // We accept anything for this (besides cancel) because the user might not know what their accept button is at the moment
    // Maybe add a "done" button at the bottom?
    bool somethingpushed = false;
    for(int i = 0; i < keys.keys.size(); i++) {
      if(i == pms->buttons[BUTTON_CANCEL])
        continue;
      if(keys.keys[i].push)
        somethingpushed = true;
    }
    if(somethingpushed) {
      (*current_mode) = true;
    } else if(keys.keys[pms->buttons[BUTTON_CANCEL]].push || (*current_button) == -1) {
      pms->choicemode = CHOICE_IDLE;
      (*current_button) = -1;
    }
  }
}

void standardButtonRender(const float *ystarts, int yscount, float xstart, float xend, float textsize, const int *buttons, const bool *inverts, const vector<vector<string> > &names, int sel_button, bool sel_button_reading, float fadeFactor, char prefixchar) {
  int linesneeded = 0;
  for(int i = 0; i < names.size(); i++)
    linesneeded += names[i].size();
  CHECK(linesneeded <= yscount - 1);
  int cy = 1 + (yscount - linesneeded + 1) / 2;
  for(int i = 0; i < names.size(); i++) {
    if(sel_button == i && !sel_button_reading) {
      setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
    } else {
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
    }
    for(int j = 0; j < names[i].size(); j++)
      drawText(names[i][j], textsize, xstart, ystarts[cy++]);
    string btext;
    if(sel_button == i && sel_button_reading) {
      btext = "???";
      setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
    } else if(buttons[i] == -1) {
      btext = "";
    } else {
      if(prefixchar == 'B') {
        CHECK(!inverts);
        btext = StringPrintf("%c%02d", prefixchar, buttons[i]);
      } else if(prefixchar == 'X') {
        CHECK(inverts);
        btext = StringPrintf("%c%d%c", prefixchar, buttons[i], inverts[i] ? '-' : '+');
      } else {
        CHECK(0);
      }
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
    }
    drawJustifiedText(btext.c_str(), textsize, xend, ystarts[cy - 1], TEXT_MAX, TEXT_MIN);
  }
  CHECK(cy <= yscount);
}

class RenderInfo {
public:
  // Basic math!
  // compiletime constants
  static const int textline_size = 7;  // How many times larger a line of text is than its divider
  static const int textline_count = 8; // How many lines we're going to have
  static const int border_size = 2;
  static const int divider_size = 3;
  static const int units = textline_size * textline_count + textline_count - 1 + border_size * 2 + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders, plus the increased divider from categories to data

  // runtime constants
  Float4 drawzone;
  float unitsize;
  float border;
  float xstart;
  float xend;
  vector<float> ystarts;
    
  RenderInfo(const Float4 &in_dz) {
    drawzone = in_dz;    

    // runtime constants
    unitsize = drawzone.y_span() / units;
    border = unitsize * border_size;
    xstart = drawzone.sx + unitsize * border_size;
    xend = drawzone.ex - unitsize * border_size;
    ystarts.resize(textline_count);
    ystarts[0] = drawzone.sy + unitsize * border_size;
    for(int i = 1; i < textline_count; i++)
      ystarts[i] = drawzone.sy + unitsize * (border_size + divider_size + i - 1) + unitsize * textline_size * i;
  }
};

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  if(!pms->faction) { // if player hasn't chosen faction yet
    pms->fireHeld = 0;
    {
      Float2 dir = deadzone(keys.menu, 0, 0.2) * 0.01;
      dir.y *= -1;
      pms->compasspos += dir;
    }
    int targetInside = -1;
    for(int j = 0; j < factions.size(); j++) {
      if(isInside(factions[j].compass_location, pms->compasspos) && !factions[j].taken) {
        //CHECK(targetInside == -1);
        targetInside = j;
      }
    }
    if(targetInside != -1) {            
      for(int j = 0; j < keys.keys.size(); j++) {
        if(keys.keys[j].repeat) {
          pms->faction = &factions[targetInside];
          pms->settingmode = SETTING_BUTTONS;
          pms->choicemode = CHOICE_FIRSTPASS; // Because it is.
          factions[targetInside].taken = true;
        }
      }
    }
  } else {
    pms->fireHeld -= 1;
    if(pms->fireHeld < 0)
        pms->fireHeld = 0;
    if(pms->choicemode == CHOICE_IDLE) {
      CHECK(pms->faction);
      if(keys.l.push)
        pms->settingmode--;
      if(keys.r.push)
        pms->settingmode++;
      if(pms->settingmode < 0)
        pms->settingmode = 0;
      if(pms->settingmode >= SETTING_LAST)
        pms->settingmode = SETTING_LAST - 1;
      if((keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.d.down) && pms->settingmode != SETTING_READY)
        pms->choicemode = CHOICE_ACTIVE;
      if((keys.keys[pms->buttons[BUTTON_ACCEPT]].down || keys.d.down) && pms->settingmode == SETTING_READY)
        pms->fireHeld += 2;
      if(pms->fireHeld > 60)
        pms->fireHeld = 60;
    } else if(pms->settingmode == SETTING_BUTTONS) {
      vector<float> triggers;
      for(int i = 0; i < keys.keys.size(); i++)
        triggers.push_back(keys.keys[i].push);
      standardButtonTick(pms->buttons, NULL, BUTTON_LAST, &pms->setting_button_current, &pms->setting_button_reading, keys, triggers, pms);
    } else if(pms->settingmode == SETTING_AXISTYPE) {
      if(keys.l.push)
        pms->traverse_axistype(-1, keys.axes.size());
      if(keys.r.push)
        pms->traverse_axistype(1, keys.axes.size());
      if(pms->choicemode == CHOICE_ACTIVE) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.keys[pms->buttons[BUTTON_CANCEL]].push || keys.u.push) {
          pms->choicemode = CHOICE_IDLE;
        }
      } else if(pms->choicemode == CHOICE_FIRSTPASS) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
          pms->settingmode++;
        }
      } else {
        CHECK(0);
      }
    } else if(pms->settingmode == SETTING_AXISCHOOSE) {
      vector<float> triggers;
      for(int i = 0; i < keys.axes.size(); i++)
        triggers.push_back(keys.axes[i]);
      standardButtonTick(pms->axes, pms->axes_invert, 2, &pms->setting_axis_current, &pms->setting_axis_reading, keys, triggers, pms);
    } else if(pms->settingmode == SETTING_TEST) {
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push) {
        if(pms->choicemode == CHOICE_FIRSTPASS) {
          pms->settingmode++;
        } else {
          pms->choicemode = CHOICE_IDLE;
        }
      }
    } else if(pms->settingmode == SETTING_READY) {
      pms->choicemode = CHOICE_IDLE; // There is no other option! Idle is the only possibility! Bow down before your god!
    } else {
      CHECK(0);
    }
  }
  
  // this is kind of hacky
  if(pms->settingmode == SETTING_TEST && !pms->test_game) {
    CHECK(!pms->test_player);
    
    pms->test_player = new Player(pms->faction->faction, 0);
    
    const RenderInfo rin(pms->faction->compass_location);
    
    Float4 boundy = Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]);
    boundy -= boundy.midpoint();
    
    float mn = min(boundy.x_span(), boundy.y_span());
    boundy *= (100 / mn);
    
    pms->test_game = new Game();
    pms->test_game->initDemo(pms->test_player, boundy);
  }
  
  // so is this
  if(pms->settingmode == SETTING_TEST) {
    if(pms->choicemode == CHOICE_IDLE) {
      vector<Keystates> kst(1);
      CHECK(!pms->test_game->runTick(kst));
    } else if(pms->choicemode == CHOICE_ACTIVE || pms->choicemode == CHOICE_FIRSTPASS) {
      vector<Keystates> kst;
      kst.push_back(genKeystate(keys, *pms));
      CHECK(!pms->test_game->runTick(kst));
    } else {
      CHECK(0);
    }
  }
}

void runSettingRender(const PlayerMenuState &pms) {
  if(!pms.faction) {
    setColor(1.0, 1.0, 1.0);
    //char bf[16];
    //sprintf(bf, "p%d", i);
    if(pms.compasspos.x != 0 || pms.compasspos.y != 0) {
      drawLine(pms.compasspos.x, pms.compasspos.y - 0.06, pms.compasspos.x, pms.compasspos.y - 0.02, 0.004);
      drawLine(pms.compasspos.x, pms.compasspos.y + 0.06, pms.compasspos.x, pms.compasspos.y + 0.02, 0.004);
      drawLine(pms.compasspos.x - 0.06, pms.compasspos.y, pms.compasspos.x - 0.02, pms.compasspos.y, 0.004);
      drawLine(pms.compasspos.x + 0.06, pms.compasspos.y, pms.compasspos.x + 0.02, pms.compasspos.y, 0.004);
    }
    //drawText(bf, 20, pms.compasspos.x + 5, pms.compasspos.y + 5);
  } else {
    
    const RenderInfo rin(pms.faction->compass_location);
    const float fadeFactor = (60 - pms.fireHeld) / 60.;
    
    {
      const float nameFade = pow(1.0 - fadeFactor, 2);
      setColor(pms.faction->faction->color * nameFade);
      
      vector<string> text = pms.faction->faction->name_lines;
      text.push_back("");
      text.push_back("Ready");
      
      drawJustifiedMultiText(text, rin.textline_size * rin.unitsize, rin.unitsize, rin.drawzone.midpoint(), TEXT_CENTER, TEXT_CENTER);
    }
    
    setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
    {
      vector<Float2> rectish;
      rectish.push_back(Float2(rin.drawzone.sx + rin.border, rin.drawzone.sy));
      rectish.push_back(Float2(rin.drawzone.ex - rin.border, rin.drawzone.sy));
      rectish.push_back(Float2(rin.drawzone.ex, rin.drawzone.sy + rin.border));
      rectish.push_back(Float2(rin.drawzone.ex, rin.drawzone.ey - rin.border));
      rectish.push_back(Float2(rin.drawzone.ex - rin.border, rin.drawzone.ey));
      rectish.push_back(Float2(rin.drawzone.sx + rin.border, rin.drawzone.ey));
      rectish.push_back(Float2(rin.drawzone.sx, rin.drawzone.ey - rin.border));
      rectish.push_back(Float2(rin.drawzone.sx, rin.drawzone.sy + rin.border));
      drawLineLoop(rectish, 0.002);
    }
    
    {
      // Topic line!
      setColor(pms.faction->faction->color * fadeFactor);
      drawDvec2(pms.faction->faction->icon, Float4(rin.xstart, rin.ystarts[0], rin.xstart + rin.unitsize * rin.textline_size, rin.ystarts[0] + rin.unitsize * rin.textline_size), 50, 0.003);
      
      const int activescale = 4;
      float txstart = rin.xstart + rin.unitsize * rin.textline_size + rin.border * 2;
      float title_units = (rin.xend - txstart) / (SETTING_LAST - 1 + activescale);
      
      int units = 0;
    
      for(int i = 0; i < SETTING_LAST; i++) {
        bool active = (i == pms.settingmode);
        
        int tunits = active ? activescale : 1;
        string text = active ? setting_names[i] : string() + setting_names[i][0];
        setColor(((active && pms.choicemode == CHOICE_IDLE) ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5)) * fadeFactor);
        
        drawJustifiedText(text, rin.textline_size * rin.unitsize, title_units * (units + tunits / 2.) + txstart, rin.ystarts[0], TEXT_CENTER, TEXT_MIN);
        
        units += tunits;
      }
      
      CHECK(units == SETTING_LAST - 1 + activescale);
      
    }
    
    if(pms.settingmode == SETTING_BUTTONS) {
      vector<vector<string> > names;
      for(int i = 0; i < BUTTON_LAST; i++) {
        vector<string> tix;
        tix.push_back(button_names_a[i]);
        tix.push_back(button_names_b[i]);
        names.push_back(tix);
      }
      standardButtonRender(&rin.ystarts[0], rin.textline_count, rin.xstart, rin.xend, rin.textline_size * rin.unitsize, pms.buttons, NULL, names, pms.setting_button_current, pms.setting_button_reading, fadeFactor, 'B');
    } else if(pms.settingmode == SETTING_AXISTYPE) {
      if(pms.choicemode != CHOICE_IDLE)
        setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
      else
        setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      drawJustifiedText(ksax_names[pms.setting_axistype], rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[2], TEXT_CENTER, TEXT_MIN);
      
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      drawJustifiedText(ksax_descriptions[pms.setting_axistype][0], rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[4], TEXT_CENTER, TEXT_MIN);
      drawJustifiedText(ksax_descriptions[pms.setting_axistype][1], rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[5], TEXT_CENTER, TEXT_MIN);
      
      drawJustifiedText("l/r changes mode", rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      
      // TODO: better pictorial representations
    } else if(pms.settingmode == SETTING_AXISCHOOSE) {
      CHECK(ksax_axis_names[pms.setting_axistype].size() == BUTTON_LAST);
      standardButtonRender(&rin.ystarts[0], rin.textline_count, rin.xstart, rin.xend, rin.textline_size * rin.unitsize, pms.axes, pms.axes_invert, ksax_axis_names[pms.setting_axistype], pms.setting_axis_current, pms.setting_axis_reading, fadeFactor, 'X');
    } else if(pms.settingmode == SETTING_TEST) {
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      if(pms.choicemode == CHOICE_IDLE) {
        drawJustifiedText("Fire to test", rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      } else {
        drawJustifiedText("Cancel when done", rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      }
      GfxWindow gfxw(Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]));
      pms.test_game->renderToScreen();
    } else if(pms.settingmode == SETTING_READY) {
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      const char * const text[] = {"Hold fire when", "ready. Let go", "to cancel.", "", "Move left/right to", "change config."};
      for(int i = 0; i < 6; i++)
        drawJustifiedText(text[i], rin.textline_size * rin.unitsize, (rin.xstart + rin.xend) / 2, rin.ystarts[i + 2], TEXT_CENTER, TEXT_MIN);
    } else {
      CHECK(0);
    }
  }
}

bool Metagame::runTick(const vector<Controller> &keys) {
  CHECK(keys.size() == pms.size());
  if(mode == MGM_PLAYERCHOOSE) {
    for(int i = 0; i < keys.size(); i++)
      runSettingTick(keys[i], &pms[i], factions);
    {
      int readyusers = 0;
      int chosenusers = 0;
      for(int i = 0; i < pms.size(); i++) {
        if(pms[i].readyToPlay())
          readyusers++;
        if(pms[i].faction)
          chosenusers++;
      }
      if(readyusers == chosenusers && chosenusers >= 2) {
        mode = MGM_FACTIONTYPE;
        playerdata.clear();
        playerdata.resize(readyusers);
        findLevels(playerdata.size());
        int pid = 0;
        for(int i = 0; i < pms.size(); i++) {
          if(pms[i].faction) {
            playerdata[pid] = Player(pms[i].faction->faction, 0);
            pid++;
          }
        }
        CHECK(pid == playerdata.size());
        game.initChoice(&playerdata);
      }
    }
  } else if(mode == MGM_FACTIONTYPE) {
    if(game.runTick(genKeystates(keys, pms)) || FLAGS_factionMode != -1) {
      if(FLAGS_factionMode != -1)
        faction_mode = FLAGS_factionMode;
      else
        faction_mode = game.winningTeam();
      if(faction_mode == -1) {
        vector<int> teams = game.teamBreakdown();
        CHECK(teams.size() == 5);
        teams.erase(teams.begin() + 4);
        int smalteam = *min_element(teams.begin(), teams.end());
        vector<int> smalteams;
        for(int i = 0; i < teams.size(); i++)
          if(teams[i] == smalteam)
            smalteams.push_back(i);
        faction_mode = smalteams[int(frand() * smalteams.size())];
      }
      CHECK(faction_mode >= 0 && faction_mode < FACTION_LAST);
      int pid = 0;  // Reset players
      for(int i = 0; i < pms.size(); i++) {
        if(pms[i].faction) {
          playerdata[pid] = Player(pms[i].faction->faction, faction_mode);
          pid++;
        }
      }
      CHECK(pid == playerdata.size());
      mode = MGM_SHOP;
      currentShop = 0;
      shop = Shop(&playerdata[0]);
      
      calculateLrStats();
      lrCash.clear();
      lrCash.resize(playerdata.size());
    }
  } else if(mode == MGM_SHOP) {
    vector<Keystates> ki = genKeystates(keys, pms);
    if(currentShop == -1) {
      // this is a bit hacky - SHOP mode when currentShop is -1 is the "show results" screen
      for(int i = 0; i < ki.size(); i++)
        if(ki[i].fire.repeat)
          checked[i] = true;
      if(count(checked.begin(), checked.end(), false) == 0) {
        for(int i = 0; i < playerdata.size(); i++)
          playerdata[i].addCash(lrCash[i]);
        currentShop = 0;
        shop = Shop(&playerdata[0]);
      }
    } else if(shop.runTick(ki[currentShop])) {
      // and here's our actual shop - the tickrunning happens in the conditional, this is just what happens if it's time to change shops
      currentShop++;
      if(currentShop != playerdata.size()) {
        shop = Shop(&playerdata[currentShop]);
      } else {
        mode = MGM_PLAY;
        game.initStandard(&playerdata, levels[int(frand() * levels.size())], &win_history, faction_mode);
        CHECK(win_history.size() == gameround);
      }
    }
  } else if(mode == MGM_PLAY) {
    if(game.runTick(genKeystates(keys, pms))) {
      gameround++;
      if(gameround % roundsBetweenShop == 0) {
        mode = MGM_SHOP;
        currentShop = -1;
        calculateLrStats();
        checked.clear();
        checked.resize(playerdata.size());
      } else {
        float firepower = game.firepowerSpent;
        game.initStandard(&playerdata, levels[int(frand() * levels.size())], &win_history, faction_mode);
        game.firepowerSpent = firepower;
      }
    }
  } else {
    CHECK(0);
  }
  return false;
}

vector<Ai *> distillAi(const vector<Ai *> &ai, const vector<PlayerMenuState> &players) {
  CHECK(ai.size() == players.size());
  vector<Ai *> rv;
  for(int i = 0; i < players.size(); i++)
    if(players[i].faction)
      rv.push_back(ai[i]);
  return rv;
}
  
void Metagame::ai(const vector<Ai *> &ai) const {
  CHECK(ai.size() == pms.size());
  if(mode == MGM_PLAYERCHOOSE) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updateCharacterChoice(factions, pms, i);
  } else if(mode == MGM_FACTIONTYPE) {
    game.ai(distillAi(ai, pms));
  } else if(mode == MGM_SHOP) {
    if(currentShop == -1) {
      for(int i = 0; i < ai.size(); i++)
        if(ai[i])
          ai[i]->updateWaitingForReport();
    } else {
      CHECK(currentShop >= 0 && currentShop < playerdata.size());
      shop.ai(distillAi(ai, pms)[currentShop]);
    }
  } else if(mode == MGM_PLAY) {
    game.ai(distillAi(ai, pms));
  } else {
    CHECK(0);
  }
}

void Metagame::renderToScreen() const {
  if(mode == MGM_PLAYERCHOOSE) {
    setZoomCenter(0, 0, 1.1);
    setColor(1.0, 1.0, 1.0);
    //drawRect(Float4(-(4./3), -1, (4./3), 1), 0.001);
    for(int i = 0; i < pms.size(); i++) {
      runSettingRender(pms[i]);
    }
    for(int i = 0; i < factions.size(); i++) {
      if(!factions[i].taken) {
        setColor(factions[i].faction->color);
        drawDvec2(factions[i].faction->icon, squareInside(factions[i].compass_location), 50, 0.003);
        //drawRect(factions[i].compass_location, 0.003);
      }
    }
  } else if(mode == MGM_FACTIONTYPE) {
    game.renderToScreen();
    if(!controls_users()) {    
      setColor(1.0, 1.0, 1.0);
      setZoom(0, 0, 100);
      drawText(StringPrintf("faction setting round"), 2, 5, 82);
    }
  } else if(mode == MGM_SHOP) {
    if(currentShop == -1) {
      setZoom(0, 0, 600);
      setColor(1.0, 1.0, 1.0);
      drawText("damage", 30, 20, 20);
      drawText("kills", 30, 20, 80);
      drawText("wins", 30, 20, 140);
      drawText("base", 30, 20, 200);
      drawText("totals", 30, 20, 320);
      drawMultibar(lrCategory[0], Float4(200, 20, 700, 60));
      drawMultibar(lrCategory[1], Float4(200, 80, 700, 120));
      drawMultibar(lrCategory[2], Float4(200, 140, 700, 180));
      drawMultibar(lrCategory[3], Float4(200, 200, 700, 240));
      drawMultibar(lrPlayer, Float4(200, 320, 700, 360));
      setColor(1.0, 1.0, 1.0);
      drawJustifiedText("waiting", 30, 400, 400, TEXT_CENTER, TEXT_MIN);
      int notdone = count(checked.begin(), checked.end(), false);
      CHECK(notdone);
      int cpos = 0;
      float increment = 800.0 / notdone;
      for(int i = 0; i < checked.size(); i++) {
        if(!checked[i]) {
          setColor(playerdata[i].getFaction()->color);
          drawDvec2(playerdata[i].getFaction()->icon, boxAround(Float2((cpos + 0.5) * increment, float(440 + 580) / 2), min(increment * 0.95f, float(580 - 440)) / 2), 50, 1);
          cpos++;
        }
      }
    } else {
      shop.renderToScreen();
    }
  } else if(mode == MGM_PLAY) {
    game.renderToScreen();
    if(!controls_users()) {    
      setColor(1.0, 1.0, 1.0);
      setZoom(0, 0, 100);
      drawText(StringPrintf("round %d", gameround), 2, 5, 82);
    }
  } else {
    CHECK(0);
  }
}

void Metagame::calculateLrStats() {
  vector<vector<float> > values(4);
  for(int i = 0; i < playerdata.size(); i++) {
    values[0].push_back(playerdata[i].consumeDamage());
    values[1].push_back(playerdata[i].consumeKills());
    values[2].push_back(playerdata[i].consumeWins());
    values[3].push_back(1);
    dprintf("%d: %f %f %f", i, values[0].back(), values[1].back(), values[2].back());
  }
  vector<float> totals(values.size());
  for(int j = 0; j < totals.size(); j++) {
    totals[j] = accumulate(values[j].begin(), values[j].end(), 0.0);
  }
  int chunkTotal = 0;
  for(int i = 0; i < totals.size(); i++) {
    if(totals[i] > 1e-6)
      chunkTotal++;
  }
  dprintf("%d, %f\n", gameround, game.firepowerSpent);
  long double totalReturn = 75 * powl(1.08, gameround) * playerdata.size() * roundsBetweenShop + game.firepowerSpent * 0.8;
  dprintf("Total cash is %s", stringFromLongdouble(totalReturn).c_str());
  
  if(totalReturn > 1e3000) {
    totalReturn = 1e3000;
    dprintf("Adjusted to 1e3000\n");
  }
  
  for(int i = 0; i < playerdata.size(); i++) {
    for(int j = 0; j < totals.size(); j++) {
      if(totals[j] > 1e-6)
        values[j][i] /= totals[j];
    }
  }
  // values now stores percentages for each category
  
  vector<float> playercash(playerdata.size());
  for(int i = 0; i < playercash.size(); i++) {
    for(int j = 0; j < totals.size(); j++) {
      playercash[i] += values[j][i];
    }
    playercash[i] /= chunkTotal;
  }
  // playercash now stores percentages for players
  
  vector<Money> playercashresult(playerdata.size());
  for(int i = 0; i < playercash.size(); i++) {
    playercashresult[i] = Money(playercash[i] * totalReturn);
  }
  // playercashresult now stores cashola for players
  
  lrCategory = values;
  lrPlayer = playercash;
  lrCash = playercashresult;
  
}

void Metagame::drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const {
  CHECK(sizes.size() == playerdata.size());
  float total = accumulate(sizes.begin(), sizes.end(), 0.0);
  if(total < 1e-6) {
    dprintf("multibar failed, total is %f\n", total);
    return;
  }
  vector<pair<float, int> > order;
  for(int i = 0; i < sizes.size(); i++)
    order.push_back(make_pair(sizes[i], i));
  sort(order.begin(), order.end());
  reverse(order.begin(), order.end());
  float width = dimensions.ex - dimensions.sx;
  float per = width / total;
  float cpos = dimensions.sx;
  float barbottom = (dimensions.ey - dimensions.sy) * 3 / 4 + dimensions.sy;
  float iconsize = (dimensions.ey - dimensions.sy) / 4 * 0.9;
  for(int i = 0; i < order.size(); i++) {
    if(order[i].first == 0)
      continue;
    setColor(playerdata[order[i].second].getFaction()->color);
    float epos = cpos + order[i].first * per;
    drawShadedRect(Float4(cpos, dimensions.sy, epos, barbottom), 1, 6);
    drawDvec2(playerdata[order[i].second].getFaction()->icon, boxAround(Float2((cpos + epos) / 2, (dimensions.ey + (dimensions.ey - iconsize)) / 2), iconsize / 2), 20, 0.1);
    cpos = epos;
  }
}

void Metagame::findLevels(int playercount) {
  if(!levels.size()) {
    ifstream ifs("data/levels/levellist.txt");
    string line;
    while(getLineStripped(ifs, &line)) {
      Level lev = loadLevel("data/levels/" + line);
      if(lev.playersValid.count(playercount))
        levels.push_back(lev);
    }
    dprintf("Got %d usable levels\n", levels.size());
    CHECK(levels.size());
  }
}

class distanceFrom {
  public:
  
  int ycenter_;
  distanceFrom(int ycenter) : ycenter_(ycenter) { };
  
  float dist(pair<int, int> dt) const {
    return dt.first * dt.first * PHI * PHI + dt.second * dt.second;
  }
  
  bool operator()(pair<int, int> lhs, pair<int, int> rhs) const {
    return dist(lhs) < dist(rhs);
  }
};

pair<Float4, vector<Float2> > getFactionCenters(int fcount) {
  for(int rows = 1; ; rows++) {
    float y_size = 2. / rows;
    float x_size = y_size * PHI;
  
    // the granularity of x_bounds is actually one half x
    int x_bounds = (int)((4./3.) / x_size * 2) - 1;
    {
      float xbt = (x_bounds + 2) * x_size / 2;
      dprintf("x_bounds is a total of %f, %f in each direction", xbt, xbt / 2);
    }
    
    int center_y = -!(rows % 2);
    
    int starrow = -(rows - 1) / 2;
    int endrow = rows / 2 + 1;
    CHECK(endrow - starrow == rows);
    dprintf("%d is %d to %d\n", rows, starrow, endrow);
    
    vector<pair<int, int> > icents;
    
    for(int i = starrow; i < endrow; i++)
      for(int j = -x_bounds; j <= x_bounds; j++)
        if(!(j % 2) == !(i % 2) && (i || j))
          icents.push_back(make_pair(j, center_y + i * 2));
    
    sort(icents.begin(), icents.end(), distanceFrom(center_y));
    icents.erase(unique(icents.begin(), icents.end()), icents.end());
    dprintf("At %d generated %d\n", rows, icents.size());
    if(icents.size() >= fcount) {
      pair<Float4, vector<Float2> > rv;
      rv.first = Float4(-x_size / 2, -y_size / 2, x_size / 2, y_size / 2);
      rv.first *= 0.9;
      for(int i = 0; i < fcount; i++) {
        rv.second.push_back(Float2(icents[i].first * x_size / 2, icents[i].second * y_size / 2));
        Float4 synth = rv.first + rv.second.back();
        CHECK(abs(synth.sx) <= 4./3);
        CHECK(abs(synth.ex) <= 4./3);
      }
      CHECK(rv.second.size() == fcount);
      return rv;
    }
  }
}

DEFINE_int(debugControllers, 0, "Number of controllers to set to debug defaults");

Metagame::Metagame(int playercount, int in_roundsBetweenShop) {
  
  faction_mode = -1;
  roundsBetweenShop = in_roundsBetweenShop;
  CHECK(roundsBetweenShop >= 1);
  
  pms.clear();
  pms.resize(playercount, PlayerMenuState(Float2(0, 0)));
  
  {
    const vector<IDBFaction> &facts = factionList();
    
    for(int i = 0; i < facts.size(); i++) {
      FactionState fs;
      fs.taken = false;
      fs.faction = &facts[i];
      // fs.compass_location will be written later
      factions.push_back(fs);
    }
  }
  
  pair<Float4, vector<Float2> > factcents = getFactionCenters(factions.size());
  for(int i = 0; i < factcents.second.size(); i++)
    factions[i].compass_location = factcents.first + factcents.second[i];
  
  mode = MGM_PLAYERCHOOSE;
  
  gameround = 0;
  
  CHECK(FLAGS_debugControllers >= 0 && FLAGS_debugControllers <= 2);
  CHECK(factions.size() >= FLAGS_debugControllers);
  
  if(FLAGS_debugControllers >= 1) {
    CHECK(pms.size() >= 1); // better be
    pms[0].faction = &factions[0];
    factions[0].taken = true;
    pms[0].settingmode = SETTING_READY;
    pms[0].choicemode = CHOICE_IDLE;
    pms[0].buttons[0] = 4;
    pms[0].buttons[1] = 8;
    pms[0].axes[0] = 0;
    pms[0].axes[1] = 1;
    pms[0].axes_invert[0] = false;
    pms[0].axes_invert[1] = false;
    pms[0].setting_axistype = KSAX_STEERING;
    pms[0].fireHeld = 0;
  }
  if(FLAGS_debugControllers >= 2) {
    CHECK(pms.size() >= 2); // better be
    pms[1].faction = &factions[1];
    factions[1].taken = true;
    pms[1].settingmode = SETTING_READY;
    pms[1].choicemode = CHOICE_IDLE;
    pms[1].buttons[0] = 4;
    pms[1].buttons[1] = 1;
    pms[1].axes[0] = 0;
    pms[1].axes[1] = 1;
    pms[1].axes_invert[0] = false;
    pms[1].axes_invert[1] = false;
    pms[1].setting_axistype = KSAX_ABSOLUTE;
    pms[1].fireHeld = 0;
  }

}
