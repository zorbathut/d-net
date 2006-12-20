
#include "shop.h"

#include "ai.h"
#include "args.h"
#include "gfx.h"
#include "player.h"
#include "audio.h"

using namespace std;

float ShopLayout::options_vspan() const {
  return getZoom().ey - itemheight() + fontsize() / 2 - voffset();
}

float ShopLayout::options_equip_vspan() const {
  return options_vspan() - itemheight() * 3;
}

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

float ShopLayout::expandy(int tier) const {
  CHECK(tier >= 0);
  if(tier < int_expandy.size())
    return int_expandy[tier];
  return 1.0;
}

float ShopLayout::scrollpos(int tier) const {
  CHECK(tier >= 0);
  if(tier < int_scroll.size())
    return int_scroll[tier].first;
  return 0.0;
}

pair<bool, bool> ShopLayout::scrollmarkers(int tier) const {
  CHECK(tier >= 0);
  if(tier < int_scroll.size())
    return int_scroll[tier].second;
  return make_pair(false, false);
}

Float2 ShopLayout::equip1(int depth) const {
  return Float2(hoffbase(depth) + int_boxborder, int_boxborder);
}
Float2 ShopLayout::equip2(int depth) const {
  return Float2(hoffbase(depth) + int_boxwidth - int_boxborder, int_boxborder);
}

float ShopLayout::implantUpgradeDiff() const {
  return int_boxwidth / 8;
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
  
  int_pricehpos = int_boxwidth - int_boxborder;
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

void ShopLayout::updateScroll(const vector<int> &curpos, const vector<int> &options, const vector<float> &height) {
  if(int_scroll.size() < options.size()) {
    int_scroll.resize(options.size(), make_pair(0, make_pair(false, false)));
  }
  
  vector<int> vcurpos = curpos;
  if(vcurpos.size() < int_scroll.size())
    vcurpos.resize(int_scroll.size(), 0);
  
  vector<int> voptions = options;
  if(voptions.size() < int_scroll.size())
    voptions.resize(int_scroll.size(), 0);
  
  vector<float> vheight = height;
  if(vheight.size() < int_scroll.size())
    vheight.resize(int_scroll.size(), 1);
  
  CHECK(vcurpos.size() == int_scroll.size());
  CHECK(vcurpos.size() == voptions.size());
  
  for(int i = 0; i < vcurpos.size(); i++) {
    int max_rows = (int)floor(vheight[i] / int_itemheight) - 2;
    
    float diff = abs(int_scroll[i].first - (vcurpos[i] - max_rows / 2));
    diff = diff / 30;
    if(diff < 0.05)
      diff = 0;
    int_scroll[i].first = clamp(approach(int_scroll[i].first, vcurpos[i] - max_rows / 2, diff), 0, max(0, voptions[i] - max_rows));
    int_scroll[i].second.first = (int_scroll[i].first != 0);
    int_scroll[i].second.second = (int_scroll[i].first != max(0, voptions[i] - max_rows));
  }
}

DEFINE_bool(cullShopTree, true, "Cull items which the players wouldn't want or realistically can't yet buy");

void Shop::renormalize(HierarchyNode &item, const Player *player, int playercount, Money highestCash) {
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

  for(int i = 0; i < item.branches.size(); i++) {
    bool keep = true;
    
    // Various upgrade-related stuff
    if(keep && item.branches[i].type == HierarchyNode::HNT_UPGRADE) {
      
      // If the upgrade isn't available, we don't include it
      if(keep && !player->isUpgradeAvailable(item.branches[i].upgrade))
        keep = false;
      
      // If there's a prereq and the player doesn't own it, we don't include it
      if(keep && item.branches[i].upgrade->prereq && !player->hasUpgrade(item.branches[i].upgrade->prereq)) {
        CHECK(player->isUpgradeAvailable(item.branches[i].upgrade->prereq)); // we do make sure the player *could* own it, just for safety's sake
        keep = false;
      }
      
      // If there's a postreq and the player already has this one, we don't include it
      if(keep && item.branches[i].upgrade->has_postreq && player->hasUpgrade(item.branches[i].upgrade))
        keep = false;
    }
    
    // More prereqs with the implant slot
    if(keep && item.branches[i].type == HierarchyNode::HNT_IMPLANTSLOT) {
      // If there's a prereq and the player doesn't own it, we don't include it
      if(keep && item.branches[i].implantslot->prereq && !player->hasImplantSlot(item.branches[i].implantslot->prereq))
        keep = false;
      
      // If there's a postreq and the player already has this one, we don't include it
      if(keep && item.branches[i].implantslot->has_postreq && player->hasImplantSlot(item.branches[i].implantslot))
        keep = false;
    }
    
    // If this is the Bombardment category, and there's only 2 players, we get rid of it entirely (TODO: how do you sell bombardment if you're stuck with 2 players?)
    if(keep && item.branches[i].type == HierarchyNode::HNT_CATEGORY && item.branches[i].cat_restrictiontype == HierarchyNode::HNT_BOMBARDMENT && playercount <= 2)
      keep = false;
    
    // If the item isn't supposed to spawn yet, we get rid of it.
    if(keep && item.branches[i].spawncash > highestCash)
      keep = false;
    
    if(keep) {
      renormalize(item.branches[i], player, playercount, highestcash);
      
      // Now that the subitem is normalized, we see if we need to eliminate it anyway. This only applies to categories.
      if(item.branches[i].type == HierarchyNode::HNT_CATEGORY) {
        // If we have tanks, bombardment, or glory devices, and there's only one item left, it's the default item.
        if(item.branches[i].cat_restrictiontype == HierarchyNode::HNT_BOMBARDMENT || item.branches[i].cat_restrictiontype == HierarchyNode::HNT_GLORY || item.branches[i].cat_restrictiontype == HierarchyNode::HNT_TANK) {
          CHECK(item.branches[i].branches.size() > 0);
          if(item.branches[i].branches.size() == 1)
            keep = false;
        } else if(item.branches[i].cat_restrictiontype == HierarchyNode::HNT_UPGRADE || item.branches[i].cat_restrictiontype == HierarchyNode::HNT_IMPLANT_CAT) {
          if(item.branches[i].branches.size() == 0)
            keep = false;
        }
      }
    }
    
    if(!keep) {
      item.branches.erase(item.branches.begin() + i);
      i--;
    }
  }
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

void drawScrollBar(const ShopLayout &slay, int depth, float ofs, bool bottom) {
  Float4 zone = slay.box(depth);
  zone.sy += ofs;
  zone.ey += ofs;
  
  string text;
  if(!bottom) {
    text = "Up";
  } else {
    text = "Down";
  }
  
  // Center
  {
    Float4 czone = zone;
    czone.sx = (zone.sx * 2 + zone.ex) / 3;
    czone.ex = (zone.sx + zone.ex * 2) / 3;
    drawSolid(czone);
    setColor(C::box_border);
    drawRect(czone, slay.boxthick());
    setColor(C::active_text);
    drawJustifiedText(text, slay.fontsize(), czone.midpoint(), TEXT_CENTER, TEXT_CENTER);
  }
  
  vector<float> beef;
  beef.push_back((zone.sx * 5 + zone.ex) / 6);
  beef.push_back((zone.sx + zone.ex * 5) / 6);
  for(int i = 0; i  < beef.size(); i++) {
    vector<Float2> tri;
    if(bottom) {
      tri.push_back(Float2(beef[i], zone.ey));
      tri.push_back(Float2(beef[i] + zone.y_span(), zone.sy));
      tri.push_back(Float2(beef[i] - zone.y_span(), zone.sy));
    } else {
      tri.push_back(Float2(beef[i], zone.sy));
      tri.push_back(Float2(beef[i] + zone.y_span(), zone.ey));
      tri.push_back(Float2(beef[i] - zone.y_span(), zone.ey));
    }
    drawSolidLoop(tri);
    drawLineLoop(tri, slay.boxthick());
  }
}

bool normalizeSelling(bool selling, HierarchyNode::Type type) {
  if(type == HierarchyNode::HNT_IMPLANTSLOT || type == HierarchyNode::HNT_IMPLANTITEM || type == HierarchyNode::HNT_IMPLANTITEM_UPG || type == HierarchyNode::HNT_EQUIPWEAPON)
    return false;
  else
    return selling;
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
      rendpos.push_back(make_pair(i, Float2(0, slay.voffset() + ((i - slay.scrollpos(depth)) * slay.itemheight()) * slay.expandy(depth))));
    for(int i = node.branches.size() - 1; i > desiredfront; i--)
      rendpos.push_back(make_pair(i, Float2(0, slay.voffset() + ((i - slay.scrollpos(depth)) * slay.itemheight()) * slay.expandy(depth))));
    rendpos.push_back(make_pair(desiredfront, Float2(0, slay.voffset() + ((desiredfront - slay.scrollpos(depth)) * slay.itemheight()) * slay.expandy(depth))));
  }
  
  if(depth < curloc.size())
    renderNode(node.branches[curloc[depth]], depth + 1, player);
  
  Float4 boundbox = Float4(slay.box(depth).sx - slay.fontsize() / 2, slay.voffset(), slay.box(depth).ex + slay.fontsize() / 2, slay.voffset());
  if(slay.scrollmarkers(depth).first) {
    drawScrollBar(slay, depth, slay.voffset(), false);
    boundbox.sy += slay.itemheight();
  }
  
  if(node.type == HierarchyNode::HNT_EQUIP) {
    boundbox.ey = boundbox.ey + slay.options_equip_vspan();
  } else {
    boundbox.ey = boundbox.ey + slay.options_vspan();
  }
  
  float bbxey = boundbox.ey;
  
  if(slay.scrollmarkers(depth).second) {
    drawScrollBar(slay, depth, boundbox.ey - slay.box(depth).y_span(), true);
    boundbox.ey -= slay.itemheight();
  }
  
  // this must happen before the gfxwindow
  if(node.type == HierarchyNode::HNT_EQUIP) {
    float maxdown = -1000;
    for(int i = 0; i < rendpos.size(); i++)
      maxdown = max(maxdown, rendpos[i].second.y + slay.itemheight());
    maxdown = min(maxdown, bbxey);
    
    maxdown -= slay.itemheight();
    
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
  
  GfxWindow gfxw(boundbox, 1.0);
  
  for(int j = 0; j < rendpos.size(); j++) {
    const int itemid = rendpos[j].first;
    const bool effectiveselling = normalizeSelling(selling, node.branches[itemid].type);
    
    if(effectiveselling) {
      setColor(Color(0.8, 0, 0));
    } else {
      setColor(C::box_border);
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
    } else if(node.branches[itemid].displaymode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
      Float4 box = slay.box(depth);
      box.sx += slay.implantUpgradeDiff();
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
    } else if(node.branches[itemid].displaymode == HierarchyNode::HNDM_IMPLANT_UPGRADE) {
      drawText("Level " + roman_number(player->implantLevel(node.branches[itemid].implantitem)) + " upgrade", slay.fontsize(), slay.description(depth) + rendpos[j].second + Float2(slay.implantUpgradeDiff(), 0));
      if(!effectiveselling)
        drawJustifiedText(node.branches[itemid].cost(player).textual().c_str(), slay.fontsize(), slay.price(depth) + rendpos[j].second, TEXT_MAX, TEXT_MIN);
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
        display = StringPrintf("%s", node.branches[itemid].cost(player).textual().c_str());
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
  
  const bool effectiveselling = normalizeSelling(selling, getCurNode().type);
  
  if(getCurNode().type == HierarchyNode::HNT_EQUIPWEAPON) {
    // EquipWeapon works differently
    
    for(int i = 0; i < SIMUL_WEAPONS; i++) {
      if(keys.fire[i].push) {
        queueSound(S::choose);
        player->setWeaponEquipBit(getCurNode().equipweapon, i, !player->getWeaponEquipBit(getCurNode().equipweapon, i));
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
            if(player->canContinue()) {
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
          if(player->canContinue()) {
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
    vector<float> height;
    for(int i = 0; i <= curloc.size(); i++) {
      options.push_back(getStepNode(i).branches.size());
      if(getStepNode(i).type == HierarchyNode::HNT_EQUIP) {
        height.push_back(slay.options_equip_vspan());
      } else {
        height.push_back(slay.options_vspan());
      }
    }
    slay.updateScroll(curloc, options, height);
  }
  
  if(hasInfo(getCurNode().type))
    cshopinf.runTick();
  
  hierarchroot = itemDbRoot();
  renormalize(hierarchroot, player, playercount, highestcash);
  
  return false;
}

void Shop::ai(Ai *ais, const Player *player) const {
  if(ais)
    ais->updateShop(player, hierarchroot);
}


void Shop::doTableRender(const Player *player) const {
  renderNode(hierarchroot, 0, player);
};

void Shop::renderToScreen(const Player *player) const {
  CHECK(player);
  //clearFrame(player->getFaction()->color * 0.05 + Color(0.02, 0.02, 0.02));
  setColor(1.0, 1.0, 1.0);
  setZoom(Float4(0, 0, 133.333, 133.333 / getAspect()));
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
    drawText(StringPrintf("Cash available: %s", v.c_str()), slay.fontsize(), Float2(80, 1));
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
    const float diff = pos.y_span() * ofs;
    pos.sx += diff;
    pos.sy += diff;
    pos.ex -= diff;
    pos.ey -= diff;
    drawDvec2(player->getFaction()->icon, pos, 50, 0.5);
  }
  doTableRender(player);

  if(hasInfo(getCurNode().type)) {
    cshopinf.renderFrame(slay.hud(), slay.fontsize(), slay.demo(), player);
  }
  
  if(getCurNode().type == HierarchyNode::HNT_SELL) {
    CHECK(curloc.size() == 1);
    Float4 bds = slay.box(curloc.size());
    bds.sy = slay.voffset();
    bds.ey = getZoom().ey - slay.voffset();
    bds = contract(bds, getTextBoxBorder(slay.fontsize()));
    
    string text = "Select this to enter Sell mode. Select this again to leave Sell mode. You may also toggle Sell mode by pressing your \"cancel\" button.";
    float height = getFormattedTextHeight(text, slay.fontsize(), bds.x_span());
    
    setColor(C::inactive_text);
    Float4 rebds(bds.sx, (bds.ey - bds.sy - height) / 2 + bds.sy, bds.ex, (bds.ey - bds.sy + height) / 2 + bds.sy);
    drawTextBoxAround(rebds, slay.fontsize());
    drawFormattedText(text, slay.fontsize(), rebds);
  }
}

// Not a valid state
Shop::Shop() { }

void Shop::init(bool in_miniature, const Player *player, int in_playercount, Money in_highestCash) {
  curloc.clear();
  cshopinf.clear();
  
  curloc.push_back(0);
  selling = false;
  disabled = false;
  
  miniature = in_miniature;
  slay = ShopLayout(miniature);
  
  hierarchroot = itemDbRoot();
  playercount = in_playercount;
  highestcash = in_highestCash;
  
  renormalize(hierarchroot, player, playercount, highestcash);
}
