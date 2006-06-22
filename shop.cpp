
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

const float sl_pricehpos = sl_boxwidth / 4 * 3;
const float sl_quanthpos = sl_boxwidth / 5 * 3;

const float sl_boxthick = 0.1;

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
    // highlight if this one is in our "active path"
    if(depth < curloc.size() && curloc[depth] == itemid) {
      if(selling) {
        setColor(1.0, 0.3, 0.3);
      } else {
        setColor(1.0, 1.0, 1.0);
      }
    } else {
      if(selling) {
        setColor(0.3, 0.05, 0.05);
      } else {
        setColor(0.3, 0.3, 0.3);
      }
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
    setColor(1.0, 1.0, 1.0);
    drawText(node.branches[itemid].name.c_str(), sl_fontsize, hoffbase + sl_boxborder, rendpos[j].second + sl_boxborder);
    // Display ammo count
    {
      if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON) {
        if(player->ammoCount(node.branches[itemid].weapon) == -1) {
          drawText(StringPrintf("%5s", "UNL"), sl_fontsize, hoffbase + sl_quanthpos, rendpos[j].second + sl_boxborder);
        } else if(player->ammoCount(node.branches[itemid].weapon) > 0) {
          drawText(StringPrintf("%5d", player->ammoCount(node.branches[itemid].weapon)), sl_fontsize, hoffbase + sl_quanthpos, rendpos[j].second + sl_boxborder);
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
        else
          CHECK(0);
        if(state == ITEMSTATE_UNOWNED) {
        } else if(state == ITEMSTATE_BOUGHT) {
          display = "bought";
          displayset = true;
        } else if(state == ITEMSTATE_EQUIPPED) {
          display = "equipped";
          displayset = true;
        }
      }
      
      // If it's not unique, or it is and it just hasn't been bought, we show the cost.
      if(!displayset && (dispmode == HierarchyNode::HNDM_COST || dispmode == HierarchyNode::HNDM_COSTUNIQUE)) {
        display = StringPrintf("%6s", node.branches[itemid].cost(player).textual().c_str());
        displayset = true;
      }
      
      // If it's a pack, we show pack quantities.
      if(!displayset && dispmode == HierarchyNode::HNDM_PACK) {
        display = StringPrintf("%dpk", node.branches[itemid].pack);
        displayset = true;
      }
      
      // If it's blank we do fucking nothing
      if(dispmode == HierarchyNode::HNDM_BLANK) {
        displayset = true;
      }
      
      CHECK(displayset);
      
      // Draw what we've got.
      drawText(display, sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder);
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
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        CHECK(node.branches[itemid].type == HierarchyNode::HNT_WEAPON);
        if(!player->canSellWeapon(node.branches[itemid].weapon)) {
          dispmode = HierarchyNode::HNDM_COSTUNIQUE;
        }
      }
      
      if(dispmode == HierarchyNode::HNDM_BLANK) {
      } else if(dispmode == HierarchyNode::HNDM_COST) {
        setColor(1.0, 0.3, 0.3);
        drawText(StringPrintf("%6s", node.branches[itemid].sellvalue(player).textual().c_str()), sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawText(StringPrintf("%dpk", node.branches[itemid].pack), sl_fontsize, hoffbase + sl_pricehpos, rendpos[j].second + sl_boxborder);
      } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
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

void approach(float *write, float target, float delta) {
  if(abs(*write - target) <= delta)
    *write = target;
  else if(*write < target)
    *write += delta;
  else if(*write > target)
    *write -= delta;
  else
    CHECK(0);  // oh god bear is driving car
}

void Shop::doTableUpdate() {
  float nxofs = max((int)curloc.size() - 1 - !getCurNode().branches.size(), 0);
  approach(&xofs, nxofs, framechange);

  int sz = max(expandy.size(), curloc.size() + 1);
  expandy.resize(sz, 1.0);
  vector<float> nexpandy(sz, 1.0);
  if(!getCurNode().branches.size() && curloc.size() >= 2)
    nexpandy[curloc.size() - 2] = 0.0;
  for(int i = 0; i < expandy.size(); i++)
    approach(&expandy[i], nexpandy[i], framechange);
};

void Shop::doTableRender() const {
  renderNode(itemDbRoot(), 0);
};

void Shop::renderToScreen() const {
  CHECK(player);
  clearFrame(player->getFaction()->color * 0.05 + Color(0.02, 0.02, 0.02));
  setColor(1.0, 1.0, 1.0);
  setZoom(0, 0, 100);
  drawText(StringPrintf("cash onhand %s", player->getCash().textual().c_str()), 2, 80, 1);
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
  expandy.resize(2, 1.0); // not really ideal but hey
  xofs = 0;
  selling = false;
  disabled = false;
}
