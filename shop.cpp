
#include "shop.h"

#include "ai.h"
#include "args.h"
#include "gfx.h"
#include "player.h"
#include "audio.h"

using namespace std;

DEFINE_bool(cullShopTree, true, "Cull items which the players wouldn't want or realistically can't yet buy");

bool sortByTankCost(const HierarchyNode &lhs, const HierarchyNode &rhs) {
  CHECK(lhs.type == HierarchyNode::HNT_TANK);
  CHECK(rhs.type == HierarchyNode::HNT_TANK);
  return lhs.tank->base_cost > rhs.tank->base_cost;
}

void Shop::renormalize(HierarchyNode &item, const Player *player, int playercount, Money highestCash) {
  if(item.type == HierarchyNode::HNT_EQUIP) {
    item.branches.clear();
    
    vector<vector<const IDBWeapon *> > weaps = player->getWeaponList();
    for(int i = 0; i < weaps.size(); i++) {
      {
        HierarchyNode hod;
        hod.type = HierarchyNode::HNT_EQUIPCATEGORY;
        hod.cat_restrictiontype = HierarchyNode::HNT_EQUIP_CAT;
        hod.displaymode = HierarchyNode::HNDM_BLANK;
        hod.buyable = false;
        if(i < SIMUL_WEAPONS) {
          hod.name = StringPrintf("Weapon system %d", i);
        } else if(i == WMSPC_UNEQUIPPED) {
          hod.name = "Offline weapons";
        } else if(i == WMSPC_NEW) {
          hod.name = "New weapons";
        } else {
          CHECK(0);
        }
        item.branches.push_back(hod);
      }
      for(int j = 0; j < weaps[i].size(); j++) {
        HierarchyNode hod;
        hod.type = HierarchyNode::HNT_EQUIPWEAPON;
        hod.cat_restrictiontype = HierarchyNode::HNT_EQUIP_CAT;
        hod.displaymode = HierarchyNode::HNDM_EQUIP;
        hod.buyable = true;
        hod.name = weaps[i][j]->name;
        hod.pack = 1;
        hod.equipweapon = weaps[i][j];
        item.branches.push_back(hod);
      }
    }

    vector<string> errors;
    item.checkConsistency(&errors);
    if(errors.size()) {
      dprintf("oh god renormalized hierarchy broke how can this be\n");
      for(int i = 0; i < errors.size(); i++)
        dprintf("    %s\n", errors[i].c_str());
      CHECK(0);
    }
  }

  for(int i = 0; i < item.branches.size(); i++) {
    bool keep = true;
    
    HierarchyNode &titem = item.branches[i];
    
    // Various upgrade-related stuff
    if(keep && titem.type == HierarchyNode::HNT_UPGRADE) {
      
      // If the upgrade isn't available, we don't include it
      if(keep && !player->isUpgradeAvailable(titem.upgrade))
        keep = false;
      
      // If there's a prereq and the player doesn't own it, we don't include it
      if(keep && titem.upgrade->prereq && !player->hasUpgrade(titem.upgrade->prereq)) {
        CHECK(player->isUpgradeAvailable(titem.upgrade->prereq)); // we do make sure the player *could* own it, just for safety's sake
        keep = false;
      }
      
      // If there's a postreq and the player already has this one, we don't include it
      if(keep && titem.upgrade->has_postreq && player->hasUpgrade(titem.upgrade))
        keep = false;
    }
    
    // More prereqs with the implant slot
    if(keep && titem.type == HierarchyNode::HNT_IMPLANTSLOT) {
      // If there's a prereq and the player doesn't own it, we don't include it
      if(keep && titem.implantslot->prereq && !player->hasImplantSlot(titem.implantslot->prereq))
        keep = false;
      
      // If there's a postreq and the player already has this one, we don't include it
      if(keep && titem.implantslot->has_postreq && player->hasImplantSlot(titem.implantslot))
        keep = false;
    }
    
    // If this is the Bombardment category, and there's only 2 players, we get rid of it entirely (TODO: how do you sell bombardment if you're stuck with 2 players?)
    if(keep && FLAGS_cullShopTree && titem.type == HierarchyNode::HNT_CATEGORY && titem.cat_restrictiontype == HierarchyNode::HNT_BOMBARDMENT && playercount <= 2)
      keep = false;
    
    // If the item isn't supposed to spawn yet, we get rid of it.
    if(keep && titem.spawncash > highestCash)
      keep = false;
    
    if(keep) {
      renormalize(titem, player, playercount, highestcash);
      
      // Now that the subitem is normalized, we see if we need to eliminate it anyway. This only applies to categories.
      if(titem.type == HierarchyNode::HNT_CATEGORY) {

        if(titem.cat_restrictiontype == HierarchyNode::HNT_GLORY || titem.cat_restrictiontype == HierarchyNode::HNT_TANK) {
          // If we have tanks or glory devices, and there's only one item left, it's the default item.
          CHECK(titem.branches.size() > 0);
          if(titem.branches.size() == 1)
            keep = false;
        } else if(titem.cat_restrictiontype == HierarchyNode::HNT_BOMBARDMENT) {
          // If it's bombardment, life is more complicated. 
          if(titem.branches.size() == 0)
            keep = false;
          if(titem.branches.size() == 1 && titem.branches[0].type == HierarchyNode::HNT_BOMBARDMENT && titem.branches[0].bombardment == defaultBombardment())
            keep = false;
          if(titem.branches.size() == 1 && titem.branches[0].type == HierarchyNode::HNT_CATEGORY) {
            CHECK(keep);
            vector<HierarchyNode> nod = titem.branches[0].branches;
            titem.branches = nod;  // augh
          }
        } else if(titem.cat_restrictiontype == HierarchyNode::HNT_IMPLANT_CAT) {
          if(titem.branches.size() == 0)
            keep = false;
        }
      }
    }
    
    if(!keep) {
      item.branches.erase(item.branches.begin() + i);
      i--;
    }
  }
  
  if(item.type == HierarchyNode::HNT_CATEGORY && item.cat_restrictiontype == HierarchyNode::HNT_TANK)
    stable_sort(item.branches.begin(), item.branches.end(), sortByTankCost);
}

const HierarchyNode &Shop::getStepNode(int step) const {
  CHECK(step >= 0 && step <= curloc.size());
  if(step == 0) {
    return hierarchroot;
  } else {
    const HierarchyNode &nd = getStepNode(step - 1);
    CHECK(curloc[step - 1] >= 0 && curloc[step - 1] < nd.branches.size());
    return nd.branches[curloc[step - 1]];
  }
}

const HierarchyNode &Shop::getCurNode() const {
  return getStepNode(curloc.size());
}

const HierarchyNode &Shop::getCategoryNode() const {
  return getStepNode(curloc.size() - 1);
}

bool normalizeSelling(bool selling, HierarchyNode::Type type) {
  if(type == HierarchyNode::HNT_IMPLANTSLOT || type == HierarchyNode::HNT_IMPLANTITEM || type == HierarchyNode::HNT_IMPLANTITEM_UPG || type == HierarchyNode::HNT_EQUIPWEAPON || type == HierarchyNode::HNT_EQUIPCATEGORY)
    return false;
  else
    return selling;
}

Money cost(const HierarchyNode &node, const Player *player) {
  if(node.type == HierarchyNode::HNT_WEAPON) {
    return player->costWeapon(node.weapon);
  } else if(node.type == HierarchyNode::HNT_UPGRADE) {
    return player->costUpgrade(node.upgrade);
  } else if(node.type == HierarchyNode::HNT_GLORY) {
    return player->costGlory(node.glory);
  } else if(node.type == HierarchyNode::HNT_BOMBARDMENT) {
    return player->costBombardment(node.bombardment);
  } else if(node.type == HierarchyNode::HNT_TANK) {
    return player->costTank(node.tank);
  } else if(node.type == HierarchyNode::HNT_IMPLANTSLOT) {
    return player->costImplantSlot(node.implantslot);
  } else if(node.type == HierarchyNode::HNT_IMPLANTITEM_UPG) {
    return player->costImplantUpg(node.implantitem);
  } else {
    CHECK(0);
  }
}

Money sellvalue(const HierarchyNode &node, const Player *player) {
  if(node.type == HierarchyNode::HNT_WEAPON) {
    return player->sellvalueWeapon(node.weapon);
  } else if(node.type == HierarchyNode::HNT_GLORY) {
    return player->sellvalueGlory(node.glory);
  } else if(node.type == HierarchyNode::HNT_BOMBARDMENT) {
    return player->sellvalueBombardment(node.bombardment);
  } else if(node.type == HierarchyNode::HNT_TANK) {
    return player->sellvalueTank(node.tank);
  } else {
    CHECK(0);
  }
}

void Shop::renderNode(const HierarchyNode &node, int depth, const Player *player) const {
  if(depth < curloc.size())
    renderNode(node.branches[curloc[depth]], depth + 1, player);
  
  slay.dynamicZoom();
  
  slay.drawScrollMarkers(depth);
  GfxWindow gfxw(slay.getScrollBBox(depth), 1.0);
  
  if(node.type == HierarchyNode::HNT_DONE) {
    vector<string> msg = player->blockedReasons();
    if(!msg.empty()) {
      Float4 otbox = slay.textbox(depth);
      Float4 tbox;
      tbox.sx = lerp(otbox.sx, otbox.ex, 0.05);
      tbox.ex = lerp(otbox.sx, otbox.ex, 1 - 0.05);
      tbox.sy = lerp(otbox.sy, otbox.ey, 0.4);
      tbox.ey = otbox.ey;
      drawFormattedTextBox(msg, slay.fontsize(), tbox, C::active_text, C::box_border);
    }
  }
  
  ShopPlacement tsp;
  if(curloc.size() > depth)
    tsp = ShopPlacement(depth, -1, node.branches.size(), curloc[depth]);
  else
    tsp = ShopPlacement(depth, -1, node.branches.size(), 0);
  
  vector<int> renderorder = slay.renderOrder(tsp);
  
  for(int j = 0; j < renderorder.size(); j++) {
    const int itemid = renderorder[j];
    const ShopPlacement splace = ShopPlacement(tsp.depth, itemid, tsp.siblings, tsp.active);
    const bool effectiveselling = normalizeSelling(selling, node.branches[itemid].type);
    
    if(effectiveselling) {
      setColor(Color(0.8, 0, 0));
    } else {
      setColor(C::box_border*0.5);
    }
    
    if(min(slay.box(splace).ex, getZoom().ex) - max(slay.box(splace).sx, getZoom().sx) < slay.box(splace).span_x() * 0.01)
      continue;   // if we can only see 1% of this box, just don't show any of it - gets rid of some ugly rendering edge cases
    
    if(node.branches[itemid].displaymode == HierarchyNode::HNDM_EQUIP) {
      drawSolid(slay.boximplantupgrade(splace));
      drawRect(slay.boximplantupgrade(splace), slay.boxthick());
    } else if(node.branches[itemid].displaymode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
      drawSolid(slay.boximplantupgrade(splace));
      drawRect(slay.boximplantupgrade(splace), slay.boxthick());
    } else {
      drawSolid(slay.box(splace));
      drawRect(slay.box(splace), slay.boxthick());
    }
    
    // highlight if this one is in our "active path"
    if(depth < curloc.size() && curloc[depth] == itemid) {
      setColor(node.branches[itemid].getHighlightColor());
    } else {
      setColor(node.branches[itemid].getColor());
    }
    
    if(node.branches[itemid].displaymode == HierarchyNode::HNDM_EQUIP) {
      drawText(node.branches[itemid].name.c_str(), slay.fontsize(), slay.descriptionimplantupgrade(splace));
    } else if(node.branches[itemid].displaymode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
      drawText("Level " + roman_number(player->implantLevel(node.branches[itemid].implantitem)) + " upgrade", slay.fontsize(), slay.descriptionimplantupgrade(splace));
    } else {
      drawText(node.branches[itemid].name.c_str(), slay.fontsize(), slay.description(splace));
    }
    
    // Display ammo count
    {
      if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON || node.branches[itemid].type == HierarchyNode::HNT_EQUIPWEAPON) {
        const IDBWeapon *weap;
        if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON) {
          weap = node.branches[itemid].weapon;
        } else if(node.branches[itemid].type == HierarchyNode::HNT_EQUIPWEAPON) {
          weap = node.branches[itemid].equipweapon;
        } else {
          CHECK(0);
        }
        
        string text;
        {
          int pac = player->ammoCount(weap);
          if(pac == UNLIMITED_AMMO) {
            text = "UNL";
          } else if(pac == 0) {
            text = "";
          } else if(pac > 0) {
            text = StringPrintf("%d", player->ammoCount(weap));
          } else {
            CHECK(0);
          }
        }
        
        if(node.branches[itemid].type == HierarchyNode::HNT_WEAPON) {
          drawJustifiedText(text, slay.fontsize(), slay.quantity(splace), TEXT_MAX, TEXT_MIN);
        } else if(node.branches[itemid].type == HierarchyNode::HNT_EQUIPWEAPON) {
          drawJustifiedText(text, slay.fontsize(), slay.price(splace), TEXT_MAX, TEXT_MIN);
        } else {
          CHECK(0);
        }        
      }
    }
    
    if(node.branches[itemid].type == HierarchyNode::HNT_EQUIPWEAPON && node.branches[itemid].equipweapon == equipselected) {
      vector<Float2> pt;
      pt.push_back(Float2(lerp(slay.box(splace).sx, slay.boximplantupgrade(splace).sx, 0.5), slay.box(splace).sy));
      pt.push_back(Float2(lerp(slay.box(splace).sx, slay.boximplantupgrade(splace).sx, 0.5), slay.box(splace).ey));
      pt.push_back(Float2(pt[0].x + (pt[1].y - pt[0].y) / 2, lerp(slay.box(splace).sy, slay.box(splace).ey, 0.5)));
      drawSolidLoop(pt);
      drawLineLoop(pt, slay.boxthick());
    }
    
    // Figure out how we want to display the "cost" text
    if(!effectiveselling) {
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
        else if(node.branches[itemid].type == HierarchyNode::HNT_IMPLANTSLOT)
          state = player->stateImplantSlot(node.branches[itemid].implantslot);
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
        display = StringPrintf("%s", cost(node.branches[itemid], player).textual().c_str());
        displayset = true;
      }
      
      // If it's a pack, we show pack quantities.
      if(!displayset && dispmode == HierarchyNode::HNDM_PACK) {
        display = StringPrintf("%dpk", node.branches[itemid].pack);
        displayset = true;
      }
      
      if(dispmode == HierarchyNode::HNDM_IMPLANT_EQUIP) {
        if(player->hasImplant(node.branches[itemid].implantitem))
          display = "Installed";
        setColor(C::active_text);
        displayset = true;
      }
      
      if(dispmode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
        display = cost(node.branches[itemid], player).textual();
        displayset = true;
      }
      
      // If it's blank or equip we do fucking nothing
      if(dispmode == HierarchyNode::HNDM_BLANK || dispmode == HierarchyNode::HNDM_EQUIP) {
        displayset = true;
      }
      
      CHECK(displayset);
      
      // Draw what we've got.
      drawJustifiedText(display, slay.fontsize(), slay.price(splace), TEXT_MAX, TEXT_MIN);
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
        drawJustifiedText(StringPrintf("%s", sellvalue(node.branches[itemid], player).textual().c_str()), slay.fontsize(), slay.price(splace), TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_PACK) {
        drawJustifiedText(StringPrintf("%dpk", node.branches[itemid].pack), slay.fontsize(), slay.price(splace), TEXT_MAX, TEXT_MIN);
      } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
      } else if(dispmode == HierarchyNode::HNDM_EQUIP) {
      } else if(dispmode == HierarchyNode::HNDM_IMPLANT_EQUIP) {
      } else if(dispmode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
      } else {
        CHECK(0);
      }
    }
  }
}

bool Shop::hasInfo(int type) const {
  return type == HierarchyNode::HNT_WEAPON || type == HierarchyNode::HNT_EQUIPWEAPON || type == HierarchyNode::HNT_GLORY || type == HierarchyNode::HNT_BOMBARDMENT || type == HierarchyNode::HNT_UPGRADE || type == HierarchyNode::HNT_TANK || type == HierarchyNode::HNT_IMPLANTITEM || type == HierarchyNode::HNT_IMPLANTITEM_UPG || type == HierarchyNode::HNT_IMPLANTSLOT;
}

bool findEquipItem(const HierarchyNode &hrt, const IDBWeapon *weap, vector<int> *path) {
  if(hrt.type == HierarchyNode::HNT_EQUIPWEAPON && hrt.equipweapon == weap)
    return true;
  
  for(int i = 0; i < hrt.branches.size(); i++) {
    path->push_back(i);
    if(findEquipItem(hrt.branches[i], weap, path))
      return true;
    path->pop_back();
  }
  
  return false;
}

bool Shop::runTick(const Keystates &keys, Player *player) {
  if(keys.l.repeat && curloc.size() > 1) {
    queueSound(S::select);
    curloc.pop_back();
  }
  if(keys.r.repeat && getCurNode().branches.size() != 0) {
    queueSound(S::select);
    curloc.push_back(0);
  }
  if(keys.u.repeat) {
    queueSound(S::select);
    curloc.back()--;
  }
  if(keys.d.repeat) {
    queueSound(S::select);
    curloc.back()++;
  }
  curloc.back() = modurot(curloc.back(), getCategoryNode().branches.size());
  
  const bool effectiveselling = normalizeSelling(selling, getCurNode().type);
  
  if(getCurNode().type != HierarchyNode::HNT_EQUIPWEAPON && getCurNode().type != HierarchyNode::HNT_EQUIPCATEGORY)
    equipselected = NULL;
  
  if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON || getCurNode().type == HierarchyNode::HNT_EQUIPCATEGORY) {
    // EquipWeapon works differently
    
    if(equipselected) {
      if(keys.u.repeat) {
        player->moveWeaponUp(equipselected);
      }
      if(keys.d.repeat) {
        player->moveWeaponDown(equipselected);
      }
    }
    
    if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON) {
      if(keys.accept.push) {
        queueSound(S::choose);
        if(equipselected)
          equipselected = NULL;
        else
          equipselected = getCurNode().equipweapon;
      } else if(keys.cancel.push) {
        equipselected = NULL; // wheeee
      } else {
        for(int i = 0; i < SIMUL_WEAPONS; i++) {
          if(keys.fire[i].push) {
            queueSound(S::choose);
            if(equipselected)
              player->promoteWeapon(equipselected, i);
            else
              player->promoteWeapon(getCurNode().equipweapon, i);
            equipselected = NULL;
          }
        }
      }
    }
  } else if(getCurNode().type == HierarchyNode::HNT_SELL) {
    if(keys.accept.push) {
      queueSound(S::choose);
      selling = !selling;
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
      queueSound(S::choose);
      selling = !selling;
      disabled = true;
      buy = Button();
      change = Button();
    }
    
    if(buy.repeat) {
      bool ret = false;
      const Sound *sound = NULL;
      
      if(!effectiveselling) {
        if(getCurNode().buyable) {
          // Player is trying to buy something!
          
          if(getCurNode().type == HierarchyNode::HNT_DONE) {
            if(player->blockedReasons().empty()) {
              ret = true;
              sound = S::accept;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
            if(player->stateUpgrade(getCurNode().upgrade) != ITEMSTATE_UNOWNED) {
              sound = S::error;
            } else if(player->canBuyUpgrade(getCurNode().upgrade))  {
              player->buyUpgrade(getCurNode().upgrade);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
            if(player->canBuyWeapon(getCurNode().weapon)) {
              player->buyWeapon(getCurNode().weapon);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
            if(player->stateGlory(getCurNode().glory) != ITEMSTATE_UNOWNED) {
              player->equipGlory(getCurNode().glory);
              sound = S::choose;
            } else if(player->canBuyGlory(getCurNode().glory)) {
              player->buyGlory(getCurNode().glory);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT) {
            if(player->stateBombardment(getCurNode().bombardment) != ITEMSTATE_UNOWNED) {
              player->equipBombardment(getCurNode().bombardment);
              sound = S::choose;
            } else if(player->canBuyBombardment(getCurNode().bombardment)) {
              player->buyBombardment(getCurNode().bombardment);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_TANK) {
            if(player->stateTank(getCurNode().tank) != ITEMSTATE_UNOWNED) {
              player->equipTank(getCurNode().tank);
              sound = S::choose;
            } else if(player->canBuyTank(getCurNode().tank)) {
              player->buyTank(getCurNode().tank);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_IMPLANTSLOT) {
            if(player->stateImplantSlot(getCurNode().implantslot) != ITEMSTATE_UNOWNED) {
              sound = S::error;
            } else if(player->canBuyImplantSlot(getCurNode().implantslot))  {
              player->buyImplantSlot(getCurNode().implantslot);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_IMPLANTITEM) {
            if(player->canToggleImplant(getCurNode().implantitem)) {
              player->toggleImplant(getCurNode().implantitem);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else if(getCurNode().type == HierarchyNode::HNT_IMPLANTITEM_UPG) {
            if(player->canLevelImplant(getCurNode().implantitem)) {
              player->levelImplant(getCurNode().implantitem);
              sound = S::choose;
            } else {
              sound = S::error;
            }
          } else {
            CHECK(0);
          }
  
        } else {
          sound = S::null;
        }
      } else {
        if(getCurNode().type == HierarchyNode::HNT_DONE) {
          if(player->blockedReasons().empty()) {
            ret = true;
            sound = S::accept;
          } else {
            sound = S::error;
          }
        } else if(getCurNode().type == HierarchyNode::HNT_WEAPON && player->canSellWeapon(getCurNode().weapon)) {
          player->sellWeapon(getCurNode().weapon);
          sound = S::choose;
        } else if(getCurNode().type == HierarchyNode::HNT_GLORY && player->canSellGlory(getCurNode().glory)) {
          player->sellGlory(getCurNode().glory);
          sound = S::choose;
        } else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT && player->canSellBombardment(getCurNode().bombardment)) {
          player->sellBombardment(getCurNode().bombardment);
          sound = S::choose;
        } else if(getCurNode().type == HierarchyNode::HNT_TANK && player->canSellTank(getCurNode().tank)) {
          player->sellTank(getCurNode().tank);
          sound = S::choose;
        } else if(!getCurNode().buyable) {
          sound = S::null;
        } else {
          sound = S::error;
        }
      }
      
      CHECK(sound);
      queueSound(sound);
      if(ret)
        return true;
    }
  }
  
  slay.updateExpandy(curloc.size(), getCurNode().branches.size());
  {
    vector<int> options;
    for(int i = 0; i <= curloc.size(); i++)
      options.push_back(getStepNode(i).branches.size());
    slay.updateScroll(curloc, options);
  }
  
  if(hasInfo(getCurNode().type))
    cshopinf.runTick();
  
  hierarchroot = itemDbRoot();
  renormalize(hierarchroot, player, playercount, highestcash);
  
  if(equipselected) {
    curloc.clear();
    CHECK(findEquipItem(hierarchroot, equipselected, &curloc));
  }
  
  {
    bool hasinfo = true;
    if(getCurNode().type == HierarchyNode::HNT_WEAPON)
      cshopinf.initIfNeeded(getCurNode().weapon, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON)
      cshopinf.initIfNeeded(getCurNode().equipweapon, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_GLORY)
      cshopinf.initIfNeeded(getCurNode().glory, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_BOMBARDMENT)
      cshopinf.initIfNeeded(getCurNode().bombardment, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_UPGRADE)
      cshopinf.initIfNeeded(getCurNode().upgrade, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_TANK)
      cshopinf.initIfNeeded(getCurNode().tank, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_IMPLANTITEM)
      cshopinf.initIfNeeded(getCurNode().implantitem, false, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_IMPLANTITEM_UPG)
      cshopinf.initIfNeeded(getCurNode().implantitem, true, player, miniature);
    else if(getCurNode().type == HierarchyNode::HNT_IMPLANTSLOT)
      cshopinf.initIfNeeded(getCurNode().implantslot, player, miniature);
    else {
      hasinfo = false;
      cshopinf.clear();
    }
    CHECK(hasinfo == hasInfo(getCurNode().type)); // doublecheck
  }
  
  return false;
}

void Shop::ai(Ai *ais, const Player *player) const {
  if(ais)
    ais->updateShop(player, hierarchroot, (curloc.size() == 1 && curloc[0] == 0));
}


void Shop::doTableRender(const Player *player) const {
  renderNode(hierarchroot, 0, player);
};

void Shop::renderToScreen(const Player *player) const {
  CHECK(player);
  
  slay.staticZoom();
  
  setColor(1.0, 1.0, 1.0);
  {
    long long cash = player->getCash().value();
    string v;
    if(cash == 0)
      v = "0";
    else {
      while(cash) {
        if(v.size() && v.size() % 4 == 3)
          v += ',';
        v += cash % 10 + '0';
        cash /= 10;
      }
    }
    reverse(v.begin(), v.end());
    drawText(StringPrintf("Cash available: %s", v.c_str()), slay.fontsize(), slay.cashpos());
  }
  if(selling) {
    drawText("    Selling equipment", slay.fontsize(), Float2(1, 1));
  } else {
    drawText("    Buying equipment", slay.fontsize(), Float2(1, 1));
  }
  
  if(player->freeImplantSlots()) {
    setColor(C::red);
    drawText(StringPrintf("%d unused implant slot%s", player->freeImplantSlots(), player->freeImplantSlots() >= 2 ? "s" : ""), slay.fontsize(), Float2(1, getZoom().ey - 1 - slay.fontsize()));
  }
  
  {
    setColor(player->getFaction()->color * 0.5);
    const float ofs = 0.08;
    Float4 pos = getZoom();
    const float diff = pos.span_y() * ofs;
    pos.sx += diff;
    pos.sy += diff;
    pos.ex -= diff;
    pos.ey -= diff;
    drawDvec2(player->getFaction()->icon, pos, 50, 0.5);
  }
  
  doTableRender(player);

  slay.staticZoom();
  
  if(hasInfo(getCurNode().type)) {
    cshopinf.renderFrame(slay.hud(), slay.fontsize(), slay.demo(), player);
  }
  
  if(getCurNode().type == HierarchyNode::HNT_SELL) {
    /*CHECK(curloc.size() == 1);
    Float4 bds = slay.box(curloc.size());
    bds.sy = slay.voffset();
    bds.ey = getZoom().ey - slay.voffset();
    bds = contract(bds, getTextBoxBorder(slay.fontsize()));
    
    string text = "Select this to enter Sell mode. Select this again to leave Sell mode. You may also toggle Sell mode by pressing your \"cancel\" button.";
    float height = getFormattedTextHeight(text, slay.fontsize(), bds.span_x());
    
    setColor(C::inactive_text);
    Float4 rebds(bds.sx, (bds.ey - bds.sy - height) / 2 + bds.sy, bds.ex, (bds.ey - bds.sy + height) / 2 + bds.sy);
    drawTextBoxAround(rebds, slay.fontsize());
    drawFormattedText(text, slay.fontsize(), rebds);*/
  }
}

// Not a valid state
Shop::Shop() { }

void Shop::init(bool in_miniature, const Player *player, int in_playercount, Money in_highestCash, float aspectRatio) {
  curloc.clear();
  cshopinf.clear();
  
  curloc.push_back(0);
  selling = false;
  disabled = false;
  
  equipselected = NULL;
  
  miniature = in_miniature;
  slay = ShopLayout(miniature, aspectRatio);
  
  hierarchroot = itemDbRoot();
  playercount = in_playercount;
  highestcash = in_highestCash;
  
  renormalize(hierarchroot, player, playercount, highestcash);
}
