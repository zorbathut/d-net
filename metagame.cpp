
#include "metagame.h"

#include "ai.h"
#include "args.h"
#include "gfx.h"
#include "inputsnag.h"
#include "parse.h"
#include "player.h"

#include <fstream>
#include <numeric>

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
    renderNode(node.branches[curloc[depth]], depth + 1);
  
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
        } else if(state == ITEMSTATE_EQUIPPED) {
          display = "equipped";
        }
      }
      
      // If it's not unique, or it is and it just hasn't been bought, we show the cost.
      if(!display.size() && (dispmode == HierarchyNode::HNDM_COST || dispmode == HierarchyNode::HNDM_COSTUNIQUE))
        display = StringPrintf("%6s", node.branches[itemid].cost(player).textual().c_str());
      
      // If it's a pack, we show pack quantities.
      if(!display.size() && dispmode == HierarchyNode::HNDM_PACK)
        display = StringPrintf("%dpk", node.branches[itemid].pack);
      
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
        if(ki[i].accept.repeat)
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
    pms[0].buttons[2] = 4;
    pms[0].buttons[3] = 8;
    pms[0].buttons[4] = 5;
    pms[0].buttons[5] = 9;
    CHECK(pms[0].buttons.size() == 6);
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
    pms[1].buttons[0] = 2;
    pms[1].buttons[1] = 5;
    pms[1].buttons[2] = 2;
    pms[1].buttons[3] = 5;
    pms[1].buttons[4] = 1;
    pms[1].buttons[5] = 5;
    CHECK(pms[1].buttons.size() == 6);
    pms[1].axes[0] = 0;
    pms[1].axes[1] = 1;
    pms[1].axes_invert[0] = false;
    pms[1].axes_invert[1] = false;
    pms[1].setting_axistype = KSAX_ABSOLUTE;
    pms[1].fireHeld = 0;
  }

}
