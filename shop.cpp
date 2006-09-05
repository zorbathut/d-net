
#include "shop.h"

#include "ai.h"
#include "gfx.h"

Float4 ShopLayout::box(int depth) const {
  return Float4(hoffbase(depth), 0, hoffbase(depth) + int_boxwidth, int_fontsize + int_boxborder * 2);
}

Float2 ShopLayout::description(int depth) const {
  return Float2(hoffbase(depth) + int_boxborder, int_boxborder);
}
Float2 ShopLayout::quantity(int depth) const {
  return Float2(hoffbase(depth) + int_quanthpos, int_boxborder);
}

Float4 ShopLayout::hud() const {
  return Float4(int_hoffset, int_hudstart, int_hoffset + int_boxwidth, int_hudend);
}
Float4 ShopLayout::demo() const {
  return Float4(int_demoxstart, int_demoystart, int_demoxstart + int_demowidth, int_demoystart + int_demowidth);
}

float ShopLayout::hoffbase(int depth) const {
  return int_hoffset + (int_boxwidth + int_hoffset) * (depth - int_xofs);
}

ShopLayout::ShopLayout() {
  // not valid
}

ShopLayout::ShopLayout(bool miniature) {
  if(!miniature) {
    int_fontsize = 2;
    int_boxborder = 0.5;
    int_itemheight = 4;
  } else {
    int_fontsize = 3;
    int_boxborder = 0.75;
    int_itemheight = 6;
  }
  
  int_totalwidth = 133.334;
  
  int_hoffset = 1.5;
  int_voffset = 5;
    
  int_boxwidth = (int_totalwidth - int_hoffset * 3) / 2;
  
  int_pricehpos = int_boxwidth / 8 * 7;
  int_quanthpos = int_boxwidth / 5 * 3;
  
  int_demowidth = int_boxwidth * 3 / 5;
  int_demoxstart = int_hoffset + int_boxwidth * 1 / 5;
  int_demoystart = 50;
  
  int_boxthick = 0.1;
  
  int_hudstart = 10;
  int_hudend = 95;
  
  int_xofs = 0;
  int_expandy.resize(2, 1.0); // not really ideal but hey
}

const float framechange = 0.2;

void ShopLayout::updateExpandy(int depth, bool this_branches) {
  float nxofs = max(depth - 1 - !this_branches, 0);
  int_xofs = approach(int_xofs, nxofs, framechange);
  
  int sz = max((int)int_expandy.size(), depth + 1);
  int_expandy.resize(sz, 1.0);
  vector<float> nexpandy(sz, 1.0);
  if(!this_branches && depth >= 2)
    nexpandy[depth - 2] = 0.0;
  for(int i = 0; i < int_expandy.size(); i++)
    int_expandy[i] = approach(int_expandy[i], nexpandy[i], framechange);
}

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

void Shop::renderNode(const HierarchyNode &node, int depth) const {
  float hoffbase = slay.hoffbase(depth);
  
  vector<pair<int, Float2> > rendpos;
  if(node.branches.size()) {
    int desiredfront;
    if(curloc.size() <= depth)
      desiredfront = 0;
    else
      desiredfront = curloc[depth];
    for(int i = 0; i < desiredfront; i++)
      rendpos.push_back(make_pair(i, Float2(0, slay.voffset() + (i * slay.itemheight()) * slay.expandy(depth))));
    for(int i = node.branches.size() - 1; i > desiredfront; i--)
      rendpos.push_back(make_pair(i, Float2(0, slay.voffset() + (i * slay.itemheight()) * slay.expandy(depth))));
    rendpos.push_back(make_pair(desiredfront, Float2(0, slay.voffset() + (desiredfront * slay.itemheight()) * slay.expandy(depth))));
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
      float xend = hoffbase + slay.boxwidth();
      xend = min(xend, 133.334f);
      xstart = max(xstart, 0.f);
      if(xend - xstart < slay.boxwidth() * 0.01)
        continue;   // if we can only see 1% of this box, just don't show any of it - gets rid of some ugly rendering edge cases
    }
    drawSolid(slay.box(depth) + rendpos[j].second);
    drawRect(slay.box(depth) + rendpos[j].second, slay.boxthick());
    // highlight if this one is in our "active path"
    if(depth < curloc.size() && curloc[depth] == itemid) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    drawText(node.branches[itemid].name.c_str(), slay.fontsize(), slay.description(depth) + rendpos[j].second);
    // Display ammo count
    {
      if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON) {
        if(player->ammoCount(node.branches[itemid].weapon) == -1) {
          drawJustifiedText(StringPrintf("%s", "UNL"), slay.fontsize(), slay.quantity(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
        } else if(player->ammoCount(node.branches[itemid].weapon) > 0) {
          drawJustifiedText(StringPrintf("%d", player->ammoCount(node.branches[itemid].weapon)), slay.fontsize(), slay.quantity(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
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
            drawText(StringPrintf("%d", i + 1), slay.fontsize(), hoffbase + slay.pricehpos() + slay.fontsize() * i * 2, rendpos[j].second.y + slay.boxborder());
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
      drawJustifiedText(display, slay.fontsize(), hoffbase + slay.pricehpos(), rendpos[j].second.y + slay.boxborder(), TEXT_MAX, TEXT_MIN);
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
        drawJustifiedText(StringPrintf("%s", node.branches[itemid].sellvalue(player).textual().c_str()), slay.fontsize(), hoffbase + slay.pricehpos(), rendpos[j].second.y + slay.boxborder(), TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawJustifiedText(StringPrintf("%dpk", node.branches[itemid].pack), slay.fontsize(), hoffbase + slay.pricehpos(), rendpos[j].second.y + slay.boxborder(), TEXT_MAX, TEXT_MIN);
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
      cshopinf.init(getCurNode().weapon, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON)
      cshopinf.init(getCurNode().equipweapon, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_GLORY)
      cshopinf.init(getCurNode().glory, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT)
      cshopinf.init(getCurNode().bombardment, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_UPGRADE)
      cshopinf.init(getCurNode().upgrade, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_TANK)
      cshopinf.init(getCurNode().tank, player, miniature);
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
  
  slay.updateExpandy(curloc.size(), getCurNode().branches.size());
  
  if(curloc == lastloc)
    cshopinf.runTick();
  
  return false;
}

void Shop::ai(Ai *ais) const {
  if(ais)
    ais->updateShop(player);
}


void Shop::doTableRender() const {
  renderNode(itemDbRoot(), 0);
};

void Shop::renderToScreen() const {
  CHECK(player);
  //clearFrame(player->getFaction()->color * 0.05 + Color(0.02, 0.02, 0.02));
  setColor(1.0, 1.0, 1.0);
  setZoom(Float4(0, 0, 133.333, 133.333 / getAspect()));
  drawText(StringPrintf("Cash available %s", player->getCash().textual().c_str()), slay.fontsize(), 80, 1);
  if(selling) {
    drawText("    Selling equipment", slay.fontsize(), 1, 1);
  } else {
    drawText("    Buying equipment", slay.fontsize(), 1, 1);
  }
  setColor(player->getFaction()->color * 0.5);
  {
    const float ofs = 0.08;
    Float4 pos = getZoom();
    const float diff = pos.y_span() * ofs;
    pos.sx += diff;
    pos.sy += diff;
    pos.ex -= diff;
    pos.ey -= diff;
    drawDvec2(player->getFaction()->icon, pos, 50, 0.5);
  }
  doTableRender();

  if(curloc != lastloc) {
    dprintf("Curloc isn't lastloc!");
  } else {
    CHECK(curloc == lastloc);
    if(getCurNode().type == HierarchyNode::HNT_WEAPON || getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON || getCurNode().type == HierarchyNode::HNT_GLORY || getCurNode().type == HierarchyNode::HNT_BOMBARDMENT || getCurNode().type == HierarchyNode::HNT_UPGRADE || getCurNode().type == HierarchyNode::HNT_TANK) {
      cshopinf.renderFrame(slay.hud(), slay.fontsize(), slay.demo());
    }
  }
}

// Not a valid state
Shop::Shop() {
  player = NULL;
}

void Shop::init(Player *in_player, bool in_miniature) {
  curloc.clear();
  lastloc.clear();
  
  player = in_player;
  curloc.push_back(0);
  selling = false;
  disabled = false;
  
  miniature = in_miniature;
  slay = ShopLayout(miniature);
}
