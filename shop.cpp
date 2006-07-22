
#include "shop.h"

#include "ai.h"
#include "gfx.h"

const HierarchyNode &Shop::normalize(const HierarchyNode &item) const {
  if(item.type == HierarchyNode::HNT_EQUIP) {
    dynamic_equip = item;
    vector<const IDBWeapon *> weaps = player->getAvailableWeapons();
    for(int i = 0; i < weaps.size(); i++) {
      HierarchyNode hod;
      hod.type = HierarchyNode::HNT_EQUIPWEAPON;
      hod.cat_restrictiontype = HierarchyNode::HNT_EQUIPWEAPON;
      hod.displaymode = HierarchyNode::HNDM_EQUIP;
      hod.buyable = true;
      hod.name = weaps[i]->name;
      hod.pack = 1;
      hod.equipweapon = weaps[i];
      dynamic_equip.branches.push_back(hod);
    }
    dynamic_equip.checkConsistency();
    return dynamic_equip;
  }
  return item;
}

const HierarchyNode &Shop::getStepNode(int step) const {
  CHECK(step >= 0 && step <= curloc.size());
  if(step == 0) {
    return itemDbRoot();
  } else {
    const HierarchyNode &nd = getStepNode(step - 1);
    CHECK(curloc[step - 1] >= 0 && curloc[step - 1] < nd.branches.size());
    return normalize(nd.branches[curloc[step - 1]]);
  }
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

const float sl_pricehpos = sl_boxwidth / 8 * 7;
const float sl_quanthpos = sl_boxwidth / 5 * 3;

const float sl_demowidth = sl_boxwidth * 3 / 5;
const float sl_demoxstart = sl_hoffset + sl_boxwidth * 1 / 5;
const float sl_demoystart = 50;

const float sl_boxthick = 0.1;

const float sl_hudstart = 10;
const float sl_hudend = 95;

void Shop::renderNode(const HierarchyNode &node, int depth) const {
  float hoffbase = sl_hoffset + (sl_boxwidth + sl_hoffset) * (depth - xofs);
  
  vector<pair<int, float> > rendpos;
  if(node.branches.size()) {
    int desiredfront;
    if(curloc.size() <= depth)
      desiredfront = 0;
    else
      desiredfront = curloc[depth];
    for(int i = 0; i < desiredfront; i++)
      rendpos.push_back(make_pair(i, sl_voffset + (i * sl_itemheight) * expandy[depth]));
    for(int i = node.branches.size() - 1; i > desiredfront; i--)
      rendpos.push_back(make_pair(i, sl_voffset + (i * sl_itemheight) * expandy[depth]));
    rendpos.push_back(make_pair(desiredfront, sl_voffset + (desiredfront * sl_itemheight) * expandy[depth]));
  }
  
  if(depth < curloc.size())
    renderNode(normalize(node.branches[curloc[depth]]), depth + 1);
  
  for(int j = 0; j < rendpos.size(); j++) {
    const int itemid = rendpos[j].first;
    if(selling) {
      setColor(Color(0.8, 0, 0));
    } else {
      setColor(C::gray(0.5));
    }
    {
      float xstart = hoffbase;
      float xend = hoffbase + sl_boxwidth;
      xend = min(xend, 133.334f);
      xstart = max(xstart, 0.f);
      if(xend - xstart < sl_boxwidth * 0.01)
        continue;   // if we can only see 1% of this box, just don't show any of it - gets rid of some ugly rendering edge cases
    }
    drawSolid(Float4(hoffbase, rendpos[j].second, hoffbase + sl_boxwidth, rendpos[j].second + sl_fontsize + sl_boxborder * 2));
    drawRect(Float4(hoffbase, rendpos[j].second, hoffbase + sl_boxwidth, rendpos[j].second + sl_fontsize + sl_boxborder * 2), sl_boxthick);
    // highlight if this one is in our "active path"
    if(depth < curloc.size() && curloc[depth] == itemid) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    drawText(node.branches[itemid].name.c_str(), sl_fontsize, hoffbase + sl_boxborder, rendpos[j].second + sl_boxborder);
    // Display ammo count
    {
      if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON) {
        if(player->ammoCount(node.branches[itemid].weapon) == -1) {
          drawJustifiedText(StringPrintf("%s", "UNL"), sl_fontsize, hoffbase + sl_quanthpos, rendpos[j].second + sl_boxborder, TEXT_MAX, TEXT_MIN);
        } else if(player->ammoCount(node.branches[itemid].weapon) > 0) {
          drawJustifiedText(StringPrintf("%d", player->ammoCount(node.branches[itemid].weapon)), sl_fontsize, hoffbase + sl_quanthpos, rendpos[j].second + sl_boxborder, TEXT_MAX, TEXT_MIN);
        }
      }
    }
    // Figure out how we want to display the "cost" text
    if(!selling) {
      string display = "";
      bool displayset = false;
      const int dispmode = node.branches[itemid].displaymode;
      
      // If it's "unique", we check to see if it's already been bought. If it has, we show what state it's in.
      if(!display.size() && dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
        int state = 0;
        if(node.branches[itemid].type == HierarchyNode::HNT_UPGRADE)
          state = player->stateUpgrade(node.branches[itemid].upgrade);
        else if(node.branches[itemid].type == HierarchyNode::HNT_GLORY)
          state = player->stateGlory(node.branches[itemid].glory);
        else if(node.branches[itemid].type == HierarchyNode::HNT_BOMBARDMENT)
          state = player->stateBombardment(node.branches[itemid].bombardment);
        else if(node.branches[itemid].type == HierarchyNode::HNT_TANK)
          state = player->stateTank(node.branches[itemid].tank);
        else
          CHECK(0);
        if(state == ITEMSTATE_UNOWNED) {
        } else if(state == ITEMSTATE_BOUGHT) {
          display = "Bought";
          displayset = true;
        } else if(state == ITEMSTATE_EQUIPPED) {
          display = "Equipped";
          displayset = true;
        } else if(state == ITEMSTATE_UNAVAILABLE) {
          display = "Unavailable";
          displayset = true;
        }
      }
      
      // If it's not unique, or it is and it just hasn't been bought, we show the cost.
      if(!displayset && (dispmode == HierarchyNode::HNDM_COST || dispmode == HierarchyNode::HNDM_COSTUNIQUE)) {
        display = StringPrintf("%s", node.branches[itemid].cost(player).textual().c_str());
        displayset = true;
      }
      
      // If it's a pack, we show pack quantities.
      if(!displayset && dispmode == HierarchyNode::HNDM_PACK) {
        display = StringPrintf("%dpk", node.branches[itemid].pack);
        displayset = true;
      }
      
      // If it's an Equip, we show equip data.
      // This is rendered somewhat differently.
      if(dispmode == HierarchyNode::HNDM_EQUIP) {
        for(int i = 0; i < SIMUL_WEAPONS; i++) {
          if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, i) != WEB_UNEQUIPPED) {
            if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, i) == WEB_ACTIVE) {
              setColor(1.0, 0.8, 0.8);
            } else {
              setColor(0.6, 0.6, 0.6);
            }
            drawText(StringPrintf("%d", i + 1), sl_fontsize, hoffbase + sl_pricehpos + sl_fontsize * i * 2, rendpos[j].second + sl_boxborder);
          }
        }
        displayset = true;
      }
      
      // If it's blank we do fucking nothing
      if(dispmode == HierarchyNode::HNDM_BLANK) {
        displayset = true;
      }
      
      CHECK(displayset);
      
      // Draw what we've got.
      drawJustifiedText(display, sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder, TEXT_MAX, TEXT_MIN);
    } else {
      int dispmode = node.branches[itemid].displaymode;
      if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
        // Upgrades can't be sold
        //if(node.branches[itemid].type == HierarchyNode::HNT_UPGRADE && player->canSellUpgrade(node.branches[itemid].upgrade))
          //dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[itemid].type == HierarchyNode::HNT_GLORY && player->canSellGlory(node.branches[itemid].glory))
          dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[itemid].type == HierarchyNode::HNT_BOMBARDMENT && player->canSellBombardment(node.branches[itemid].bombardment))
          dispmode = HierarchyNode::HNDM_COST;
        if(node.branches[itemid].type == HierarchyNode::HNT_TANK && player->canSellTank(node.branches[itemid].tank))
          dispmode = HierarchyNode::HNDM_COST;
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        CHECK(node.branches[itemid].type == HierarchyNode::HNT_WEAPON);
        if(!player->canSellWeapon(node.branches[itemid].weapon)) {
          dispmode = HierarchyNode::HNDM_COSTUNIQUE;
        }
      }
      
      if(dispmode == HierarchyNode::HNDM_BLANK) {
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        setColor(1.0, 0.3, 0.3);
        drawJustifiedText(StringPrintf("%s", node.branches[itemid].sellvalue(player).textual().c_str()), sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder, TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawJustifiedText(StringPrintf("%dpk", node.branches[itemid].pack), sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder, TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
      } else if(dispmode == HierarchyNode::HNDM_EQUIP) {
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
  
  if(curloc != lastloc) {
    lastloc = curloc;
    if(getCurNode().type == HierarchyNode::HNT_WEAPON)
      cshopinf.init(getCurNode().weapon, player);
    else if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON)
      cshopinf.init(getCurNode().equipweapon, player);
    else if(getCurNode().type == HierarchyNode::HNT_GLORY)
      cshopinf.init(getCurNode().glory, player);
    else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT)
      cshopinf.init(getCurNode().bombardment, player);
    else if(getCurNode().type == HierarchyNode::HNT_UPGRADE)
      cshopinf.init(getCurNode().upgrade, player);
    else if(getCurNode().type == HierarchyNode::HNT_TANK)
      cshopinf.init(getCurNode().tank, player);
  }
  
  if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON) {
    // EquipWeapon works differently
    selling = false;
    
    for(int i = 0; i < SIMUL_WEAPONS; i++) {
      if(keys.fire[i].push) {
        player->setWeaponEquipBit(getCurNode().equipweapon, i, !player->getWeaponEquipBit(getCurNode().equipweapon, i));
      }
    }
  } else {
    
    Button buy;
    Button change;
    
    if(0) { // change this to if(selling) for "sell button" behavior
      buy = keys.cancel;
      change = keys.accept;
    } else {
      buy = keys.accept;
      change = keys.cancel;
    }
    
    if(disabled) {
      if(!buy.down && !change.down) {
        disabled = false;
      } else {
        buy = Button();
        change = Button();
      }
    }
    
    if(change.push) {
      selling = !selling;
      disabled = true;
      buy = Button();
      change = Button();
    }
    
    if(buy.repeat) {
      if(!selling) {
        if(getCurNode().buyable) {
          // Player is trying to buy something!
          
          if(getCurNode().type == HierarchyNode::HNT_DONE) {
            if(player->canContinue())
              return true;
          } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
            if(player->stateUpgrade(getCurNode().upgrade) != ITEMSTATE_UNOWNED)
              ;
              //player->equipUpgrade(getCurNode().upgrade);
            else if(player->canBuyUpgrade(getCurNode().upgrade))
              player->buyUpgrade(getCurNode().upgrade);
          } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
            if(player->canBuyWeapon(getCurNode().weapon))
              player->buyWeapon(getCurNode().weapon);
          } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
            if(player->stateGlory(getCurNode().glory) != ITEMSTATE_UNOWNED)
              player->equipGlory(getCurNode().glory);
            else if(player->canBuyGlory(getCurNode().glory))
              player->buyGlory(getCurNode().glory);
          } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
            if(player->stateBombardment(getCurNode().bombardment) != ITEMSTATE_UNOWNED)
              player->equipBombardment(getCurNode().bombardment);
            else if(player->canBuyBombardment(getCurNode().bombardment))
              player->buyBombardment(getCurNode().bombardment);
          } else if(getCurNode().type == HierarchyNode::HNT_TANK) {
            if(player->stateTank(getCurNode().tank) != ITEMSTATE_UNOWNED)
              player->equipTank(getCurNode().tank);
            else if(player->canBuyTank(getCurNode().tank))
              player->buyTank(getCurNode().tank);
          } else {
            CHECK(0);
          }
  
        }
      } else {
        if(getCurNode().type == HierarchyNode::HNT_WEAPON && player->canSellWeapon(getCurNode().weapon)) {
          player->sellWeapon(getCurNode().weapon);
        } else if(getCurNode().type == HierarchyNode::HNT_GLORY && player->canSellGlory(getCurNode().glory)) {
          player->sellGlory(getCurNode().glory);
        } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT && player->canSellBombardment(getCurNode().bombardment)) {
          player->sellBombardment(getCurNode().bombardment);
        } else if(getCurNode().type == HierarchyNode::HNT_TANK && player->canSellTank(getCurNode().tank)) {
          player->sellTank(getCurNode().tank);
        }
      }
    }
  }
  
  doTableUpdate();
  
  if(curloc == lastloc)
    cshopinf.runTick();
  
  return false;
}

void Shop::ai(Ai *ais) const {
  if(ais)
    ais->updateShop(player);
}

const float framechange = 0.2;

void Shop::doTableUpdate() {
  float nxofs = max((int)curloc.size() - 1 - !getCurNode().branches.size(), 0);
  xofs = approach(xofs, nxofs, framechange);

  int sz = max(expandy.size(), curloc.size() + 1);
  expandy.resize(sz, 1.0);
  vector<float> nexpandy(sz, 1.0);
  if(!getCurNode().branches.size() && curloc.size() >= 2)
    nexpandy[curloc.size() - 2] = 0.0;
  for(int i = 0; i < expandy.size(); i++)
    expandy[i] = approach(expandy[i], nexpandy[i], framechange);
};

void Shop::doTableRender() const {
  renderNode(itemDbRoot(), 0);
};

void Shop::renderToScreen() const {
  CHECK(player);
  clearFrame(player->getFaction()->color * 0.05 + Color(0.02, 0.02, 0.02));
  setColor(1.0, 1.0, 1.0);
  setZoom(0, 0, 100);
  drawText(StringPrintf("Cash available %s", player->getCash().textual().c_str()), 2, 80, 1);
  if(selling) {
    drawText("    Selling equipment", 2, 1, 1);
  } else {
    drawText("    Buying equipment", 2, 1, 1);
  }
  setColor(player->getFaction()->color * 0.5);
  {
    const float ofs = 8;
    drawDvec2(player->getFaction()->icon, Float4(ofs, ofs, 125 - ofs, 100 - ofs), 50, 0.5);
  }
  doTableRender();

  if(curloc != lastloc) {
    dprintf("Curloc isn't lastloc!");
  } else {
    CHECK(curloc == lastloc);
    if(getCurNode().type == HierarchyNode::HNT_WEAPON || getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON || getCurNode().type == HierarchyNode::HNT_GLORY || getCurNode().type == HierarchyNode::HNT_BOMBARDMENT || getCurNode().type == HierarchyNode::HNT_UPGRADE || getCurNode().type == HierarchyNode::HNT_TANK) {
      cshopinf.renderFrame(Float4(sl_hoffset, sl_hudstart, sl_hoffset + sl_boxwidth, sl_hudend), sl_fontsize, Float4(sl_demoxstart, sl_demoystart, sl_demoxstart + sl_demowidth, sl_demoystart + sl_demowidth));
    }
  }
}

// Not a valid state
Shop::Shop() {
  player = NULL;
}

void Shop::init(Player *in_player) {
  curloc.clear();
  expandy.clear();
  lastloc.clear();
  
  player = in_player;
  curloc.push_back(0);
  expandy.resize(2, 1.0); // not really ideal but hey
  xofs = 0;
  selling = false;
  disabled = false;
}
