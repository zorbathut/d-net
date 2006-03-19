
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

class Faction {
public:
  string filename;
  Color color;
};

const Faction factions[] = {
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

const int factioncount = sizeof(factions) / sizeof(Faction);

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
  clearFrame(player->color * 0.05 + Color(0.05, 0.05, 0.05));
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
  setColor(player->color * 0.5);
  {
    const float ofs = 8;
    drawDvec2(player->faction_symb, Float4(ofs, ofs, 125 - ofs, 100 - ofs), 0.5);
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
  firekey = 10000;
  symbol = 100;
  compasspos = Float2(0,0);
  axismode = -30;   // values that are likely to break stuff badly
}

PlayerMenuState::PlayerMenuState(Float2 cent) {
  firekey = -1;
  symbol = -1;
  compasspos = cent;
  axismode = KSAX_UDLR;
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

bool Metagame::runTick( const vector< Controller > &keys ) {
  CHECK(keys.size() == pms.size());
  if(mode == MGM_PLAYERCHOOSE) {
    for(int i = 0; i < keys.size(); i++) {
      if(pms[i].symbol == -1) { // if player hasn't chosen symbol yet
        fireHeld[i] = 0;
        pms[i].compasspos += deadzone(keys[i].menu, 0, 0.2) * 4;
        int targetInside = -1;
        for(int j = 0; j < symbolpos.size(); j++)
          if(isinside(symbolpos[j], pms[i].compasspos) && symboltaken[j] == -1)
            targetInside = j;
        if(targetInside != -1) {            
          for(int j = 0; j < keys[i].keys.size(); j++) {
            if(keys[i].keys[j].repeat) {
              pms[i].symbol = targetInside;
              pms[i].firekey = j;
              symboltaken[targetInside] = i;
            }
          }
        }
      } else {  // if player has chosen symbol
        {
          int opm = pms[i].axismode;
          if(keys[i].l.repeat) {
            do {
              pms[i].axismode--;
              pms[i].axismode += KSAX_END;
              pms[i].axismode %= KSAX_END;
            } while(ksax_minaxis[pms[i].axismode] > keys[i].axes.size());
          }
          if(keys[i].r.repeat) {
            do {
              pms[i].axismode++;
              pms[i].axismode += KSAX_END;
              pms[i].axismode %= KSAX_END;
            } while(ksax_minaxis[pms[i].axismode] > keys[i].axes.size());
          }
          if(pms[i].axismode != opm)
            fireHeld[i] = 0;  // just for a bit of added safety
        }
        if(pms[i].firekey != -1 && keys[i].keys[pms[i].firekey].down) {
          fireHeld[i]++;
        } else {
          fireHeld[i] = 0;
        }
        if(fireHeld[i] > 60)
          fireHeld[i] = 60;
      }
    }
    {
      if(count(fireHeld.begin(), fireHeld.end(), 60) == symboltaken.size() - count(symboltaken.begin(), symboltaken.end(), -1) && count(fireHeld.begin(), fireHeld.end(), 60) >= 2) {
        mode = MGM_SHOP;
        currentShop = 0;
        playerdata.clear();
        playerdata.resize(count(fireHeld.begin(), fireHeld.end(), 60));
        int pid = 0;
        for(int i = 0; i < pms.size(); i++) {
          if(pms[i].symbol != -1) {
            playerdata[pid].color = factions[pms[i].symbol].color;
            playerdata[pid].faction_symb = symbols[pms[i].symbol];
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
    if(players[i].symbol != -1)
      rv.push_back(ai[i]);
  return rv;
}
  
void Metagame::ai(const vector<Ai *> &ai) const {
  CHECK(ai.size() == pms.size());
  if(mode == MGM_PLAYERCHOOSE) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updateCharacterChoice(symbolpos, pms, i);
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
      if(pms[i].symbol == -1) {
        setColor(1.0, 1.0, 1.0);
        char bf[16];
        sprintf(bf, "p%d", i);
        drawLine(pms[i].compasspos.x, pms[i].compasspos.y - 15, pms[i].compasspos.x, pms[i].compasspos.y - 5, 1.0);
        drawLine(pms[i].compasspos.x, pms[i].compasspos.y + 15, pms[i].compasspos.x, pms[i].compasspos.y + 5, 1.0);
        drawLine(pms[i].compasspos.x - 15, pms[i].compasspos.y, pms[i].compasspos.x - 5, pms[i].compasspos.y, 1.0);
        drawLine(pms[i].compasspos.x +15, pms[i].compasspos.y, pms[i].compasspos.x + 5, pms[i].compasspos.y, 1.0);
        drawText(bf, 20, pms[i].compasspos.x + 5, pms[i].compasspos.y + 5);
      } else {
        setColor(factions[pms[i].symbol].color);
        float ye = min(600. / pms.size(), 100.);
        Float4 box( 0, ye * i, ye, ye + ye * i );
        drawDvec2(symbols[pms[i].symbol], Float4(box.sx + ye / 10, box.sy + ye / 10, box.ex - ye / 10, box.ey - ye / 10), 1.0);
        setColor(Color(1.0, 1.0, 1.0) / 60 * fireHeld[i]);
        drawRect(Float4(box.sx + ye / 20, box.sy + ye / 20, box.ex - ye / 20, box.ey - ye / 20), 1);
        setColor(Color(0.8, 0.8, 0.8));
        drawText(ksax_names[pms[i].axismode], 20, ye, ye * (i + 1. / 20));
      }
    }
    CHECK(symbols.size() == factioncount);
    for(int i = 0; i < symbols.size(); i++) {
      if(symboltaken[i] == -1) {
        setColor(factions[i].color);
        drawDvec2(symbols[i], symbolpos[i], 1.0);
      }
    }
    if(count(symboltaken.begin(), symboltaken.end(), -1) <= symboltaken.size() - 2) {
      setColor(1.0, 1.0, 1.0);
      drawText("hold fire to begin", 20, 120, 560);
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
          setColor(playerdata[i].color);
          drawDvec2(playerdata[i].faction_symb, Float4(cpos * increment, 440, (cpos + 1) * increment, 580), 1);
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
    setColor(playerdata[i].color);
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

  const Float2 cent(450, 300);
  
  roundsBetweenShop = in_roundsBetweenShop;
  CHECK(roundsBetweenShop >= 1);
  
  pms.clear();
  pms.resize(playercount, PlayerMenuState(cent));
  fireHeld.resize(playercount);
  
  for(int i = 0; i < factioncount; i++) {
    symbols.push_back(loadDvec2(factions[i].filename));
  }
  
  for(int i = 0; i < 4; i++) {
    symbolpos.push_back( boxaround( makeAngle(PI * 2 * i / 4) * 100 + cent, 50 ) );
  }
  
  for(int i = 4; i < symbols.size(); i++) {
    symbolpos.push_back( boxaround( makeAngle(PI * 2 * ( i - 4 ) / ( symbols.size() - 4 )) * 225 + cent, 50 ) );
  }
  
  symboltaken.resize(symbols.size(), -1);
  
  mode = MGM_PLAYERCHOOSE;
  
  gameround = 0;

}
