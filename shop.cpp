
#include "shop.h"

#include "ai.h"
#include "gfx.h"
#include "player.h"

using namespace std;

Float4 ShopLayout::box(int depth) const {
  return Float4(hoffbase(depth), 0, hoffbase(depth) + int_boxwidth, int_fontsize + int_boxborder * 2);
}

Float2 ShopLayout::description(int depth) const {
  return Float2(hoffbase(depth) + int_boxborder, int_boxborder);
}
Float2 ShopLayout::quantity(int depth) const {
  return Float2(hoffbase(depth) + int_quanthpos, int_boxborder);
}
Float2 ShopLayout::price(int depth) const {
  return Float2(hoffbase(depth) + int_pricehpos, int_boxborder);
}
Float2 ShopLayout::equipbit(int depth, int id) const {
  return Float2(hoffbase(depth) + int_pricehpos - int_fontsize + int_fontsize * id * 2, int_boxborder);
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

Float2 ShopLayout::equip1(int depth) const {
  return Float2(hoffbase(depth) + int_boxborder, int_boxborder);
}
Float2 ShopLayout::equip2(int depth) const {
  return Float2(hoffbase(depth) + int_boxwidth - int_boxborder, int_boxborder);
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
  int_demoystart = 45;
  
  int_boxthick = 0.1;
  
  int_hudstart = 10;
  int_hudend = 95;
  
  int_xofs = 0;
  int_expandy.resize(2, 1.0); // not really ideal but hey
  
  int_equipDiff = int_boxwidth / 4;
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

void Shop::renormalize(HierarchyNode &item, const Player *player) {
  if(item.type == HierarchyNode::HNT_EQUIP) {
    item.branches.clear();
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
      item.branches.push_back(hod);
    }
    item.checkConsistency();
  }
  
  for(int i = 0; i < item.branches.size(); i++)
    renormalize(item.branches[i], player);
}

const HierarchyNode &Shop::getStepNode(int step, const Player *player) const {
  CHECK(step >= 0 && step <= curloc.size());
  if(step == 0) {
    return hierarchroot;
  } else {
    const HierarchyNode &nd = getStepNode(step - 1, player);
    CHECK(curloc[step - 1] >= 0 && curloc[step - 1] < nd.branches.size());
    return nd.branches[curloc[step - 1]];
  }
}

const HierarchyNode &Shop::getCurNode(const Player *player) const {
  return getStepNode(curloc.size(), player);
}

const HierarchyNode &Shop::getCategoryNode(const Player *player) const {
  return getStepNode(curloc.size() - 1, player);
}

void Shop::renderNode(const HierarchyNode &node, int depth, const Player *player) const {
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
    renderNode(node.branches[curloc[depth]], depth + 1, player);
  
  if(node.type == HierarchyNode::HNT_EQUIP) {
    float maxdown = -1000;
    for(int i = 0; i < rendpos.size(); i++)
      maxdown = max(maxdown, rendpos[i].second.y);
    
    for(int i = 0; i < 2; i++) {
      Float4 box = slay.box(depth);
      box = box + Float2(0, maxdown + box.y_span() * 3);
      box.ex = box.sx + slay.equipDiff();
      box.ey = box.sy + box.y_span() * 2;
      
      if(i)
        box = box + Float2(slay.box(depth).x_span() - slay.equipDiff(), 0);
      
      setColor(C::box_border);
      drawSolid(box);
      drawRect(box, slay.boxthick());
      
      setColor(C::inactive_text);
      
      vector<string> str;
      str.push_back("?");
      str.push_back("Weapon");
      if(i == 0) {
        str[0] = "Left";
      } else if(i == 1) {
        str[0] = "Right";
      } else {
        CHECK(0);
      }
      
      drawJustifiedMultiText(str, slay.fontsize(), box.midpoint(), TEXT_CENTER, TEXT_CENTER);
    }
  }
  
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
    
    if(node.branches[itemid].displaymode == HierarchyNode::HNDM_EQUIP) {
      // Equip rendering works dramatically different from others
      Float4 box = slay.box(depth);
      CHECK(SIMUL_WEAPONS == 2);
      if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, 0) == WEB_UNEQUIPPED)
        box.sx += slay.equipDiff();
      if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, 1) == WEB_UNEQUIPPED)
        box.ex -= slay.equipDiff();
      
      drawSolid(box + rendpos[j].second);
      drawRect(box + rendpos[j].second, slay.boxthick());
    } else {
      drawSolid(slay.box(depth) + rendpos[j].second);
      drawRect(slay.box(depth) + rendpos[j].second, slay.boxthick());
    }
    
    // highlight if this one is in our "active path"
    if(depth < curloc.size() && curloc[depth] == itemid) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    
    if(node.branches[itemid].displaymode == HierarchyNode::HNDM_EQUIP) {
      drawText(node.branches[itemid].name.c_str(), slay.fontsize(), slay.description(depth) + rendpos[j].second + Float2(slay.equipDiff(), 0));
      
      for(int i = 0; i < SIMUL_WEAPONS; i++) {
        string text;
        if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, i) == WEB_EQUIPPED) {
          setColor(C::inactive_text);
          text = "Equip";
        } else if(player->getWeaponEquipBit(node.branches[itemid].equipweapon, i) == WEB_ACTIVE) {
          setColor(C::active_text);
          text = "Active";
        }
        
        if(i == 0)
          drawJustifiedText(text, slay.fontsize(), slay.equip1(depth) + rendpos[j].second, TEXT_MIN, TEXT_MIN);
        else
          drawJustifiedText(text, slay.fontsize(), slay.equip2(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
      }
      continue;
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
      
      // If it's an Equip, something has gone bizarrely wrong.
      if(dispmode == HierarchyNode::HNDM_EQUIP) {
        CHECK(0);
      }
      
      // If it's blank we do fucking nothing
      if(dispmode == HierarchyNode::HNDM_BLANK) {
        displayset = true;
      }
      
      CHECK(displayset);
      
      // Draw what we've got.
      drawJustifiedText(display, slay.fontsize(), slay.price(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
    } else {
      int dispmode = node.branches[itemid].displaymode;
      if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
        // Upgrades can't be sold
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
        drawJustifiedText(StringPrintf("%s", node.branches[itemid].sellvalue(player).textual().c_str()), slay.fontsize(), slay.price(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawJustifiedText(StringPrintf("%dpk", node.branches[itemid].pack), slay.fontsize(), slay.price(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
      } else if(dispmode == HierarchyNode::HNDM_EQUIP) {
      } else {
        CHECK(0);
      }
    }
  }
}

bool Shop::hasInfo(int type) const {
  return type == HierarchyNode::HNT_WEAPON || type == HierarchyNode::HNT_EQUIPWEAPON || type == HierarchyNode::HNT_GLORY || type == HierarchyNode::HNT_BOMBARDMENT || type == HierarchyNode::HNT_UPGRADE || type == HierarchyNode::HNT_TANK;
}

bool Shop::runTick(const Keystates &keys, Player *player) {
  if(keys.l.repeat && curloc.size() > 1)
    curloc.pop_back();
  if(keys.r.repeat && getCurNode(player).branches.size() != 0)
    curloc.push_back(0);
  if(keys.u.repeat)
    curloc.back()--;
  if(keys.d.repeat)
    curloc.back()++;
  curloc.back() += getCategoryNode(player).branches.size();
  curloc.back() %= getCategoryNode(player).branches.size();
  
  if(curloc != lastloc) {
    lastloc = curloc;
    if(getCurNode(player).type == HierarchyNode::HNT_WEAPON)
      cshopinf.init(getCurNode(player).weapon, player, miniature);
    else if(getCurNode(player).type == HierarchyNode::HNT_EQUIPWEAPON)
      cshopinf.init(getCurNode(player).equipweapon, player, miniature);
    else if(getCurNode(player).type == HierarchyNode::HNT_GLORY)
      cshopinf.init(getCurNode(player).glory, player, miniature);
    else if(getCurNode(player).type == HierarchyNode::HNT_BOMBARDMENT)
      cshopinf.init(getCurNode(player).bombardment, player, miniature);
    else if(getCurNode(player).type == HierarchyNode::HNT_UPGRADE)
      cshopinf.init(getCurNode(player).upgrade, player, miniature);
    else if(getCurNode(player).type == HierarchyNode::HNT_TANK)
      cshopinf.init(getCurNode(player).tank, player, miniature);
  }
  
  if(getCurNode(player).type == HierarchyNode::HNT_EQUIPWEAPON) {
    // EquipWeapon works differently
    selling = false;
    
    for(int i = 0; i < SIMUL_WEAPONS; i++) {
      if(keys.fire[i].push) {
        player->setWeaponEquipBit(getCurNode(player).equipweapon, i, !player->getWeaponEquipBit(getCurNode(player).equipweapon, i));
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
        if(getCurNode(player).buyable) {
          // Player is trying to buy something!
          
          if(getCurNode(player).type == HierarchyNode::HNT_DONE) {
            if(player->canContinue())
              return true;
          } else if(getCurNode(player).type == HierarchyNode::HNT_UPGRADE) {
            if(player->stateUpgrade(getCurNode(player).upgrade) != ITEMSTATE_UNOWNED)
              ;
              //player->equipUpgrade(getCurNode(player).upgrade);
            else if(player->canBuyUpgrade(getCurNode(player).upgrade))
              player->buyUpgrade(getCurNode(player).upgrade);
          } else if(getCurNode(player).type == HierarchyNode::HNT_WEAPON) {
            if(player->canBuyWeapon(getCurNode(player).weapon))
              player->buyWeapon(getCurNode(player).weapon);
          } else if(getCurNode(player).type == HierarchyNode::HNT_GLORY) {
            if(player->stateGlory(getCurNode(player).glory) != ITEMSTATE_UNOWNED)
              player->equipGlory(getCurNode(player).glory);
            else if(player->canBuyGlory(getCurNode(player).glory))
              player->buyGlory(getCurNode(player).glory);
          } else if(getCurNode(player).type == HierarchyNode::HNT_BOMBARDMENT) {
            if(player->stateBombardment(getCurNode(player).bombardment) != ITEMSTATE_UNOWNED)
              player->equipBombardment(getCurNode(player).bombardment);
            else if(player->canBuyBombardment(getCurNode(player).bombardment))
              player->buyBombardment(getCurNode(player).bombardment);
          } else if(getCurNode(player).type == HierarchyNode::HNT_TANK) {
            if(player->stateTank(getCurNode(player).tank) != ITEMSTATE_UNOWNED)
              player->equipTank(getCurNode(player).tank);
            else if(player->canBuyTank(getCurNode(player).tank))
              player->buyTank(getCurNode(player).tank);
          } else {
            CHECK(0);
          }
  
        }
      } else {
        if(getCurNode(player).type == HierarchyNode::HNT_DONE) {
          if(player->canContinue())
            return true;
        } else if(getCurNode(player).type == HierarchyNode::HNT_WEAPON && player->canSellWeapon(getCurNode(player).weapon)) {
          player->sellWeapon(getCurNode(player).weapon);
        } else if(getCurNode(player).type == HierarchyNode::HNT_GLORY && player->canSellGlory(getCurNode(player).glory)) {
          player->sellGlory(getCurNode(player).glory);
        } else if(getCurNode(player).type == HierarchyNode::HNT_BOMBARDMENT && player->canSellBombardment(getCurNode(player).bombardment)) {
          player->sellBombardment(getCurNode(player).bombardment);
        } else if(getCurNode(player).type == HierarchyNode::HNT_TANK && player->canSellTank(getCurNode(player).tank)) {
          player->sellTank(getCurNode(player).tank);
        }
      }
    }
  }
  
  slay.updateExpandy(curloc.size(), getCurNode(player).branches.size());
  
  if(curloc == lastloc && hasInfo(getCurNode(player).type))
    cshopinf.runTick();
  
  renormalize(hierarchroot, player);
  
  return false;
}

void Shop::ai(Ai *ais, const Player *player) const {
  if(ais)
    ais->updateShop(player);
}


void Shop::doTableRender(const Player *player) const {
  renderNode(hierarchroot, 0, player);
};

void Shop::renderToScreen(const Player *player) const {
  CHECK(player);
  //clearFrame(player->getFaction()->color * 0.05 + Color(0.02, 0.02, 0.02));
  setColor(1.0, 1.0, 1.0);
  setZoom(Float4(0, 0, 133.333, 133.333 / getAspect()));
  drawText(StringPrintf("Cash available %s", player->getCash().textual().c_str()), slay.fontsize(), Float2(80, 1));
  if(selling) {
    drawText("    Selling equipment", slay.fontsize(), Float2(1, 1));
  } else {
    drawText("    Buying equipment", slay.fontsize(), Float2(1, 1));
  }
  {
    setColor(player->getFaction()->color * 0.5);
    const float ofs = 0.08;
    Float4 pos = getZoom();
    const float diff = pos.y_span() * ofs;
    pos.sx += diff;
    pos.sy += diff;
    pos.ex -= diff;
    pos.ey -= diff;
    drawDvec2(player->getFaction()->icon, pos, 50, 0.5);
  }
  doTableRender(player);

  if(curloc != lastloc) {
    dprintf("Curloc isn't lastloc!");
  } else {
    CHECK(curloc == lastloc);
    if(hasInfo(getCurNode(player).type)) {
      cshopinf.renderFrame(slay.hud(), slay.fontsize(), slay.demo());
    }
  }
}

// Not a valid state
Shop::Shop() { }

void Shop::init(bool in_miniature, const HierarchyNode &hnode) {
  curloc.clear();
  lastloc.clear();
  
  curloc.push_back(0);
  selling = false;
  disabled = false;
  
  miniature = in_miniature;
  slay = ShopLayout(miniature);
  
  hierarchroot = hnode;
}
