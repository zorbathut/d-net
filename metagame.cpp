
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

class FactionSource {
public:
  string filename;
  Color color;
};

const FactionSource faction_src[] = {
  { "data/faction_f.dv2", Color(1.0, 1.0, 1.0) }, // omega
  
  { "data/faction_a.dv2", Color(1.0, 0.0, 0.0) }, // pitchfork
  { "data/faction_e.dv2", Color(0.0, 1.0, 0.0) }, // serpent
  { "data/faction_h.dv2", Color(0.0, 0.0, 1.0) }, // ocean
  
  { "data/faction_b.dv2", Color(1.0, 1.0, 0.0) }, // lightning
  { "data/faction_c.dv2", Color(0.0, 1.0, 1.0) }, // H
  { "data/faction_d.dv2", Color(1.0, 0.0, 1.0) }, // compass
  { "data/faction_g.dv2", Color(0.5, 0.5, 1.0) }, // buzzsaw
  { "data/faction_i.dv2", Color(1.0, 0.5, 0.5) }, // zen
  { "data/faction_j.dv2", Color(1.0, 0.5, 0.0) }, // pincer
  { "data/faction_k.dv2", Color(0.0, 0.6, 1.0) }, // hourglass
  { "data/faction_l.dv2", Color(1.0, 0.4, 0.6) } // poison
};

const int factioncount = sizeof(faction_src) / sizeof(FactionSource);

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

const float sl_hoffset = 1.5;
const float sl_voffset = 5;

const float sl_fontsize = 2;
const float sl_boxborder = 0.5;
const float sl_itemheight = 4;

const float sl_boxwidth = 42;

const float sl_pricehpos = 29;

const float sl_boxthick = 0.1;

void Shop::renderNode(const HierarchyNode &node, int depth) const {
  CHECK(depth < 3 || node.branches.size() == 0);
  
  float hoffbase = sl_hoffset + ( sl_boxwidth + sl_hoffset ) * depth;
  
  for(int i = 0; i < node.branches.size(); i++) {
    if(depth < curloc.size() && curloc[depth] == i) {
      setColor(1.0, 1.0, 1.0);
      renderNode(node.branches[i], depth + 1);
    } else {
      setColor(0.3, 0.3, 0.3);
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
        if(node.branches[i].type == HierarchyNode::HNT_GLORY && player->glory != node.branches[i].glory)
          dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[i].type == HierarchyNode::HNT_BOMBARDMENT && player->bombardment != node.branches[i].bombardment)
          dispmode = HierarchyNode::HNDM_COST;
      }
      if(dispmode == HierarchyNode::HNDM_BLANK) {
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        drawText( StringPrintf("%6d", node.branches[i].cost), sl_fontsize, hoffbase + sl_pricehpos, sl_voffset + i * sl_itemheight + sl_boxborder );
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawText( StringPrintf("%dpk", node.branches[i].quantity), sl_fontsize, hoffbase + sl_pricehpos, sl_voffset + i * sl_itemheight + sl_boxborder );
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
  
  if(keys.f.repeat && getCurNode().buyable) {
    // Player is trying to buy something!
    
    // First check to see if we're allowed to buy it
    bool canBuy = false;
    if(getCurNode().type == HierarchyNode::HNT_DONE) {
      canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
      if(player->cash >= getCurNode().cost && !player->hasUpgrade(getCurNode().upgrade))
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
      if(player->cash >= getCurNode().cost)
        canBuy = true;
      if(player->weapon != getCurNode().weapon && player->cash + player->resellAmmoValue() >= getCurNode().cost)
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
      if(player->cash >= getCurNode().cost && getCurNode().glory != player->glory)
        canBuy = true;
    } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
      if(player->cash >= getCurNode().cost && getCurNode().bombardment != player->bombardment)
        canBuy = true;
    } else {
      CHECK(0);
    }
    
    // If so, buy it
    if(canBuy) {
      player->cash -= getCurNode().cost;
      if(getCurNode().type == HierarchyNode::HNT_DONE) {
        return true;
      } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
        player->upgrades.push_back(getCurNode().upgrade);
        player->reCalculate();
      } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
        if(player->weapon != getCurNode().weapon) {
          if(player->shotsLeft != -1)
            player->cash += player->resellAmmoValue();
          player->weapon = getCurNode().weapon;
          player->shotsLeft = 0;
        }
        player->shotsLeft += getCurNode().quantity;
        if(player->weapon == defaultWeapon())
          player->shotsLeft = -1;
      } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
        player->glory = getCurNode().glory;
      } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
        player->bombardment = getCurNode().bombardment;
      } else {
        CHECK(0);
      }
    }
    
  }
  return false;
}

void Shop::ai(Ai *ais) const {
  if(ais)
    ais->updateShop(player);
}

void Shop::renderToScreen() const {
  CHECK(player);
  clearFrame(player->faction->color * 0.05 + Color(0.05, 0.05, 0.05));
  setColor(1.0, 1.0, 1.0);
  setZoom(0, 0, 100);
  {
    char bf[128];
    sprintf(bf, "cash onhand %6d", player->cash);
    drawText(bf, 2, 80, 1);
  }
  {
    char bf[128];
    if(player->shotsLeft == -1) {
      sprintf(bf, "%10s infinite ammo", player->weapon->name.c_str());
    } else {
      sprintf(bf, "%15s %4d shots %6d resell", player->weapon->name.c_str(), player->shotsLeft, player->resellAmmoValue());
    }
    drawText(bf, 2, 1, 1);
  }
  setColor(player->faction->color * 0.5);
  {
    const float ofs = 8;
    drawDvec2(player->faction->icon, Float4(ofs, ofs, 125 - ofs, 100 - ofs), 0.5);
  }
  renderNode(itemDbRoot(), 0);
  float hudstart = itemDbRoot().branches.size() * sl_itemheight + sl_voffset + sl_boxborder;
  if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
    drawText("damage per second", 2, 1.5, hudstart);
    drawText(StringPrintf("%20.4f", getCurNode().weapon->getDamagePerSecond()), 2, 1.5, hudstart + 3);
    drawText("cost per damage", 2, 1.5, hudstart + 6);
    drawText(StringPrintf("%20.4f", getCurNode().weapon->getCostPerDamage()), 2, 1.5, hudstart + 9);
  } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
    drawText("total average damage", 2, 1.5, hudstart);
    drawText(StringPrintf("%20.4f", getCurNode().glory->getAverageDamage()), 2, 1.5, hudstart + 3);
  }
}

// Not a valid state
Shop::Shop() {
  player = NULL;
}

Shop::Shop(Player *in_player) {
  player = in_player;
  curloc.push_back(0);
}

PlayerMenuState::PlayerMenuState() {
  settingmode = -12345;
  choicemode = -12345;

  setting_button_current = -1;
  setting_button_reading = false;
  
  setting_axistype = 1237539;
  
  memset(buttons, -1, sizeof(buttons));

  firekey = 10000;
  faction = NULL;
  compasspos = Float2(0,0);
  axismode = -30;   // values that are likely to break stuff badly
}

PlayerMenuState::PlayerMenuState(Float2 cent) {
  settingmode = SETTING_COMPASS;
  choicemode = CHOICE_FIRSTPASS;
  
  setting_button_current = 0;
  setting_button_reading = true;
  
  setting_axistype = 0;
  
  memset(buttons, -1, sizeof(buttons));
  
  firekey = -1;
  faction = NULL;
  compasspos = cent;
  axismode = KSAX_UDLR;
}

bool PlayerMenuState::readyToPlay() const {
  return settingmode == SETTING_READY && fireHeld == 60;
}

void PlayerMenuState::traverse_axistype(int delta, int axes) {
  do {
    setting_axistype += delta;
    setting_axistype += KSAX_END;
    setting_axistype %= KSAX_END;
  } while(ksax_minaxis[setting_axistype] > axes);
}

vector<Keystates> genKeystates(const vector<Controller> &keys, const vector<PlayerMenuState> &modes) {
  vector<Keystates> kst;
  int pid = 0;
  for(int i = 0; i < modes.size(); i++) {
    if(modes[i].firekey != -1) {
      kst.push_back(Keystates());
      kst[pid].u = keys[i].u;
      kst[pid].d = keys[i].d;
      kst[pid].l = keys[i].l;
      kst[pid].r = keys[i].r;
      kst[pid].f = keys[i].keys[modes[i].firekey];
      kst[pid].axmode = modes[i].axismode;
      if(kst[pid].axmode == KSAX_UDLR || kst[pid].axmode == KSAX_ABSOLUTE) {
        kst[pid].ax[0] = keys[i].menu.x;
        kst[pid].ax[1] = keys[i].menu.y;
      } else if(kst[pid].axmode == KSAX_TANK) {
        CHECK(keys[i].axes.size() >= 4);
        kst[pid].ax[0] = keys[i].axes[1];
        kst[pid].ax[1] = keys[i].axes[2];
      } else {
        CHECK(0);
      }
      kst[pid].udlrax[0] = keys[i].menu.x;
      kst[pid].udlrax[1] = keys[i].menu.y;
      CHECK(keys[i].menu.x >= -1 && keys[i].menu.x <= 1);
      CHECK(keys[i].menu.y >= -1 && keys[i].menu.y <= 1);
      pid++;
    }
  }
  return kst;
}

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  CHECK(pms->choicemode != CHOICE_IDLE);
  if(pms->settingmode == SETTING_COMPASS) { // if player hasn't chosen faction yet
    pms->fireHeld = 0;
    {
      Float2 dir = deadzone(keys.menu, 0, 0.2) * 4;
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
          pms->firekey = j;
          pms->settingmode = SETTING_BUTTONS;
          pms->choicemode = CHOICE_FIRSTPASS; // Because it is.
          factions[targetInside].taken = true;
        }
      }
    }
  } else if(pms->settingmode == SETTING_BUTTONS) {
    if(pms->setting_button_current == -1) {
      pms->setting_button_current = 0;
      pms->setting_button_reading = (pms->choicemode == CHOICE_FIRSTPASS);
    }
    if(pms->setting_button_reading) {
      CHECK(pms->choicemode != CHOICE_IDLE);
      for(int i = 0; i < keys.keys.size(); i++) {
        if(keys.keys[i].push) {
          int j;
          bool noopify = false;
          for(j = 0; j < button_count; j++) {
            if(pms->buttons[j] == i) {
              if(pms->buttons[pms->setting_button_current] != -1)
                swap(pms->buttons[j], pms->buttons[pms->setting_button_current]);
              else
                noopify = true;
              break;
            }
          }
          if(noopify)
            continue;
          if(j == button_count)
            pms->buttons[pms->setting_button_current] = i;
          if(pms->choicemode == CHOICE_FIRSTPASS) {
            pms->setting_button_current++;
          } else {
            pms->setting_button_reading = false;
          }
          break;
        }
      }
      if(pms->setting_button_current == button_count) {
        // Done with the first pass here
        pms->settingmode++;
        pms->setting_button_current = -1;
      }
    } else {
      CHECK(pms->choicemode == CHOICE_ACTIVE);
      if(keys.u.repeat)
        pms->setting_button_current--;
      if(keys.d.repeat)
        pms->setting_button_current++;
      pms->setting_button_current += button_count;
      pms->setting_button_current %= button_count;
      // We accept anything for this (besides cancel) because the user might not know what their accept button is at the moment
      // Maybe add a "done" button at the bottom?
      bool somethingpushed = false;
      for(int i = 0; i < keys.keys.size(); i++) {
        if(i == pms->buttons[BUTTON_CANCEL])
          continue;
        if(keys.keys[i].push)
          somethingpushed = true;
      }
      if(somethingpushed)
        pms->setting_button_reading = true;
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push) {
        pms->choicemode = CHOICE_IDLE;
        pms->setting_button_current = -1;
      }
    }
  } else if(pms->settingmode == SETTING_AXISTYPE) {
    if(pms->choicemode == CHOICE_IDLE) {
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
        pms->choicemode = CHOICE_ACTIVE;
      }
      // deal with moving
    } else {
      if(keys.l.push)
        pms->traverse_axistype(-1, keys.axes.size());
      if(keys.r.push)
        pms->traverse_axistype(1, keys.axes.size());
      if(pms->choicemode == CHOICE_ACTIVE) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.keys[pms->buttons[BUTTON_CANCEL]].push) {
          pms->choicemode = CHOICE_IDLE;
        }
      } else if(pms->choicemode == CHOICE_FIRSTPASS) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
          pms->settingmode++;
        }
      } else {
        CHECK(0);
      }
    }
  } else if(pms->settingmode == SETTING_CUSTOMIZE) {
  } else if(pms->settingmode == SETTING_READY) {
  } else {
    CHECK(0);
  }
}

void runSettingRender(const PlayerMenuState &pms) {
  if(pms.settingmode != SETTING_COMPASS)
    CHECK(pms.faction);
  if(pms.settingmode == SETTING_COMPASS) {
    setColor(1.0, 1.0, 1.0);
    //char bf[16];
    //sprintf(bf, "p%d", i);
    drawLine(pms.compasspos.x, pms.compasspos.y - 15, pms.compasspos.x, pms.compasspos.y - 5, 1.0);
    drawLine(pms.compasspos.x, pms.compasspos.y + 15, pms.compasspos.x, pms.compasspos.y + 5, 1.0);
    drawLine(pms.compasspos.x - 15, pms.compasspos.y, pms.compasspos.x - 5, pms.compasspos.y, 1.0);
    drawLine(pms.compasspos.x +15, pms.compasspos.y, pms.compasspos.x + 5, pms.compasspos.y, 1.0);
    //drawText(bf, 20, pms.compasspos.x + 5, pms.compasspos.y + 5);
  } else if(pms.settingmode >= SETTING_BUTTONS && pms.settingmode < SETTING_LAST) {

    const Float4 drawzone = pms.faction->compass_location;
    
    // Basic math!
    // compiletime constants
    const int textline_size = 7;  // How many times larger a line of text is than the border
    const int textline_count = 6; // How many lines we're going to have
    const int border_size = 2;
    const int divider_size = 3;
    const int units = textline_size * textline_count + textline_count - 1 + border_size * 2 + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders (4), plus the increased divider from categories to data (2)
    // runtime constants
    const float unitsize = drawzone.y_span() / units;
    const float border = unitsize * border_size;
    const float xstart = drawzone.sx + unitsize * border_size;
    const float xend = drawzone.ex - unitsize * border_size;
    float ystarts[textline_count];
    ystarts[0] = drawzone.sy + unitsize * border_size;
    for(int i = 1; i < textline_count; i++)
      ystarts[i] = drawzone.sy + unitsize * (border_size + divider_size + i - 1) + unitsize * textline_size * i;
    
    setColor(Color(1.0, 1.0, 1.0));
    {
      vector<Float2> rectish;
      rectish.push_back(Float2(drawzone.sx + border, drawzone.sy));
      rectish.push_back(Float2(drawzone.ex - border, drawzone.sy));
      rectish.push_back(Float2(drawzone.ex, drawzone.sy + border));
      rectish.push_back(Float2(drawzone.ex, drawzone.ey - border));
      rectish.push_back(Float2(drawzone.ex - border, drawzone.ey));
      rectish.push_back(Float2(drawzone.sx + border, drawzone.ey));
      rectish.push_back(Float2(drawzone.sx, drawzone.ey - border));
      rectish.push_back(Float2(drawzone.sx, drawzone.sy + border));
      drawLineLoop(rectish, 1.0);
    }
    
    {
      // Topic line!
      setColor(pms.faction->color);
      drawDvec2(pms.faction->icon, Float4(xstart, ystarts[0], xstart + unitsize * textline_size, ystarts[0] + unitsize * textline_size), 0.5);
      
      const int activescale = 4;
      float txstart = xstart + unitsize * textline_size;
      float title_units = (xend - txstart) / (setting_real_count - 1 + activescale);
      
      int units = 0;
    
      for(int i = 0; i < setting_real_count; i++) {
        bool active = (i + setting_first == pms.settingmode);
        
        int tunits = active ? activescale : 1;
        string text = active ? setting_names[setting_first + i] : string() + setting_names[setting_first + i][0];
        setColor((active && pms.choicemode == CHOICE_IDLE) ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5));
        
        drawJustifiedText(text, textline_size * unitsize, title_units * (units + tunits / 2.) + txstart, ystarts[0], TEXT_CENTER, TEXT_MIN);
        
        units += tunits;
      }
      
      CHECK(units == setting_real_count - 1 + activescale);
      
    }
    
    if(pms.settingmode == SETTING_BUTTONS) {
      CHECK(button_count == 2); // just 'cause I'll need to redo spacing after these
      for(int i = 0; i < button_count; i++) {
        if(pms.setting_button_current == i && !pms.setting_button_reading) {
          setColor(Color(1.0, 1.0, 1.0));
        } else {
          setColor(Color(0.5, 0.5, 0.5));
        }
        drawText(button_names_a[i], textline_size * unitsize, xstart, ystarts[2 + i * 2]);
        drawText(button_names_b[i], textline_size * unitsize, xstart, ystarts[3 + i * 2]);
        string btext;
        if(pms.setting_button_current == i && pms.setting_button_reading) {
          btext = "[ ]";
          setColor(Color(1.0, 1.0, 1.0));
        } else if(pms.buttons[i] == -1) {
          btext = "";
        } else {
          btext = StringPrintf("B%02d", pms.buttons[i]);
          setColor(Color(0.5, 0.5, 0.5));
        }
        drawJustifiedText(btext.c_str(), textline_size * unitsize, xend, ystarts[3 + i * 2], TEXT_MAX, TEXT_MIN);
      }
    } else if(pms.settingmode == SETTING_AXISTYPE) {
      if(pms.choicemode != CHOICE_IDLE)
        setColor(Color(1.0, 1.0, 1.0));
      else
        setColor(Color(0.5, 0.5, 0.5));
      drawJustifiedText(ksax_names[pms.setting_axistype], textline_size * unitsize, (xstart + xend) / 2, ystarts[2], TEXT_CENTER, TEXT_MIN);
      // TODO: make these blink?
      // TODO: better pictorial representations
      drawJustifiedText("<", textline_size * unitsize, xstart, ystarts[2], TEXT_MIN, TEXT_MIN);
      drawJustifiedText(">", textline_size * unitsize, xend, ystarts[2], TEXT_MAX, TEXT_MIN);
    } else {
      CHECK(0);
    }

  } else {
    CHECK(0);
  }
}

bool Metagame::runTick( const vector< Controller > &keys ) {
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
        if(pms[i].settingmode != SETTING_COMPASS)
          chosenusers++;
      }
      if(readyusers == chosenusers && chosenusers >= 2) {
        mode = MGM_SHOP;
        currentShop = 0;
        playerdata.clear();
        playerdata.resize(readyusers);
        int pid = 0;
        for(int i = 0; i < pms.size(); i++) {
          if(pms[i].faction) {
            playerdata[pid].faction = pms[i].faction;
            pid++;
          }
        }
        CHECK(pid == playerdata.size());
        shop = Shop(&playerdata[0]);
      }
    }
  } else if(mode == MGM_SHOP) {
    vector<Keystates> ki = genKeystates(keys, pms);
    if(currentShop == -1) {
      for(int i = 0; i < ki.size(); i++)
        if(ki[i].f.repeat)
          checked[i] = true;
      if(count(checked.begin(), checked.end(), false) == 0) {
        for(int i = 0; i < playerdata.size(); i++)
          playerdata[i].cash += lrCash[i];
        currentShop = 0;
        shop = Shop(&playerdata[0]);
      }
    } else if(shop.runTick(ki[currentShop])) {
      currentShop++;
      if(currentShop != playerdata.size()) {
        shop = Shop(&playerdata[currentShop]);
      } else {
        mode = MGM_PLAY;
        for(int i = 0; i < playerdata.size(); i++) {
          playerdata[i].damageDone = 0;
          playerdata[i].kills = 0;
          playerdata[i].wins = 0;
        }
        findLevels(playerdata.size());
        game = Game(&playerdata, levels[int(frand() * levels.size())]);
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
        game = Game(&playerdata, levels[int(frand() * levels.size())]);
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
    setZoom(0, 0, 600);
    setColor(1.0, 1.0, 1.0);
    for(int i = 0; i < pms.size(); i++) {
      runSettingRender(pms[i]);
    }
    CHECK(factions.size() == factioncount);
    for(int i = 0; i < factions.size(); i++) {
      if(!factions[i].taken) {
        setColor(faction_src[i].color);
        drawDvec2(factions[i].icon, squareInside(factions[i].compass_location), 1.0);
      }
    }
    setColor(1.0, 1.0, 1.0);
    drawText("choose a faction and hold fire to begin", 20, 120, 560);
  } else if(mode == MGM_SHOP) {
    if(currentShop == -1) {
      setZoom(0, 0, 600);
      setColor(1.0, 1.0, 1.0);
      drawText("damage", 30, 20, 20);
      drawText("kills", 30, 20, 80);
      drawText("wins", 30, 20, 140);
      drawText("base", 30, 20, 200);
      drawText("totals", 30, 20, 320);
      drawMultibar(lrCategory[0], Float4(200, 20, 700, 50));
      drawMultibar(lrCategory[1], Float4(200, 80, 700, 110));
      drawMultibar(lrCategory[2], Float4(200, 140, 700, 170));
      drawMultibar(lrCategory[3], Float4(200, 200, 700, 230));
      drawMultibar(lrPlayer, Float4(200, 320, 700, 350));
      setColor(1.0, 1.0, 1.0);
      drawJustifiedText("waiting", 30, 400, 400, TEXT_CENTER, TEXT_MIN);
      int notdone = count(checked.begin(), checked.end(), false);
      CHECK(notdone);
      int cpos = 0;
      float increment = 800.0 / notdone;
      for(int i = 0; i < checked.size(); i++) {
        if(!checked[i]) {
          setColor(playerdata[i].faction->color);
          drawDvec2(playerdata[i].faction->icon, Float4(cpos * increment, 440, (cpos + 1) * increment, 580), 1);
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
    values[0].push_back(playerdata[i].damageDone);
    values[1].push_back(playerdata[i].kills);
    values[2].push_back(playerdata[i].wins);
    values[3].push_back(1);
    dprintf("%d: %f %d %d", i, playerdata[i].damageDone, playerdata[i].kills, playerdata[i].wins);
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
  float totalReturn = 75 * pow(1.08, gameround) * playerdata.size() * roundsBetweenShop + game.firepowerSpent * 0.8;
  dprintf("Total cash is %f", totalReturn);
  
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
  
  vector<int> playercashresult(playerdata.size());
  for(int i = 0; i < playercash.size(); i++) {
    playercashresult[i] = int(playercash[i] * totalReturn);
  }
  // playercashresult now stores cashola for players
  
  lrCategory = values;
  lrPlayer = playercash;
  lrCash = playercashresult;
  
}

void Metagame::drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const {
  float total = accumulate(sizes.begin(), sizes.end(), 0.0);
  if(total < 1e-6) {
    dprintf("multibar failed, total is %f\n", total);
    return;
  }
  float width = dimensions.ex - dimensions.sx;
  float per = width / total;
  float cpos = dimensions.sx;
  for(int i = 0; i < sizes.size(); i++) {
    setColor(playerdata[i].faction->color);
    float epos = cpos + sizes[i] * per;
    drawShadedRect(Float4(cpos, dimensions.sy, epos, dimensions.ey), 1, 6);
    cpos = epos;
  }
}

void Metagame::findLevels(int playercount) {
  if(!levels.size()) {
    ifstream ifs("data/levels/levellist.txt");
    string line;
    while(getLineStripped(ifs, line)) {
      Level lev = loadLevel("data/levels/" + line);
      if(lev.playersValid.count(playercount))
        levels.push_back(lev);
    }
    dprintf("Got %d usable levels\n", levels.size());
    CHECK(levels.size());
  }
}

// not a valid state
Metagame::Metagame() {
}

Metagame::Metagame(int playercount, int in_roundsBetweenShop) {

  const Float2 cent(400, 300);
  
  roundsBetweenShop = in_roundsBetweenShop;
  CHECK(roundsBetweenShop >= 1);
  
  pms.clear();
  pms.resize(playercount, PlayerMenuState(cent));
  
  for(int i = 0; i < factioncount; i++) {
    FactionState fs;
    fs.icon = loadDvec2(faction_src[i].filename);
    fs.taken = false;
    fs.color = faction_src[i].color;
    factions.push_back(fs);
  }
  
  Float4 factionbox = Float4(-160, -100, 160, 100) * 0.5;
  
  for(int i = 0; i < 4; i++) {
    Float2 loc = makeAngle(PI * 2 * i / 4) * 100 + cent;
    factions[i].compass_location = factionbox + Float4(loc, loc);
  }
  
  for(int i = 4; i < factions.size(); i++) {
    Float2 loc = makeAngle(PI * 2 * ( i - 4 ) / ( factions.size() - 4 )) * 225 + cent;
    factions[i].compass_location = factionbox + Float4(loc, loc);
  }
  
  mode = MGM_PLAYERCHOOSE;
  
  gameround = 0;

}
