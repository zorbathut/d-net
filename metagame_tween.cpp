
#include "metagame_tween.h"

#include <numeric>

#include "args.h"
#include "gfx.h"
#include "ai.h"
#include "parse.h"

using namespace std;

const float divider_ypos = 87;
const float ticker_ypos = 90;

const float ticker_text_size = 2;
const float ticker_queue_border = 1;
const float ticker_waiting_border = 1;

bool PersistentData::isPlayerChoose() const {
  return mode == TM_PLAYERCHOOSE;
}

vector<Player> &PersistentData::players() {
  return playerdata;
}

const char * const tween_textlabels[] = {"Leave/join game", "Settings", "Full shop", "Quick shop", "Done"};
enum { TTL_LEAVEJOIN, TTL_SETTINGS, TTL_FULLSHOP, TTL_QUICKSHOP, TTL_DONE, TTL_LAST };

class QueueSorter {
public:
  bool operator()(const pair<int, int> &lhs, const pair<int, int> &rhs) {
    return (lhs.second == TTL_FULLSHOP) < (rhs.second == TTL_FULLSHOP);
  }
};

bool PersistentData::tick(const vector< Controller > &keys) {
  CHECK(keys.size() == pms.size());
  
  // First: traverse and empty.
  for(int i = 0; i < slot_count; i++) {
    if(slot[i].type == Slot::EMPTY)
      continue;
    bool clear;
    if(slot[i].pid == -1) {
      clear = tickSlot(i, keys);
    } else {
      clear = tickSlot(i, vector<Controller>(1, keys[slot[i].pid]));
    }
    if(clear) {
      slot[i].type = Slot::EMPTY;
      slot[i].pid = -1;
    }
  }
  
  // Next: deal with empty slots depending on our mode.
  
  if(mode == TM_PLAYERCHOOSE) {
    CHECK(slot_count == 1);
    if(slot[0].type == Slot::EMPTY) {
      int readyusers = 0;
      for(int i = 0; i < pms.size(); i++)
        if(pms[i].readyToPlay())
          readyusers++;
      CHECK(readyusers >= 2);

      // TODO: shuffle this
      playerdata.clear();
      playerdata.resize(readyusers);
      int pid = 0;
      for(int i = 0; i < pms.size(); i++) {
        if(pms[i].faction) {
          playerdata[pid] = Player(pms[i].faction->faction, 0);
          playerid[i] = pid;
          pid++;
        }
      }
      CHECK(pid == playerdata.size());
      
      initForShop();
      return true;
    }
  } else if(mode == TM_RESULTS) {
    CHECK(slot_count == 1);
    if(slot[0].type == Slot::EMPTY)
      initForShop();
  } else if(mode == TM_SHOP) {
    // Various complications and such!
    
    // First: calculate our ugly ranges for the text labels.
    vector<pair<float, float> > ranges;
    for(int i = 0; i < TTL_LAST; i++) {
      vector<string> lines = tokenize(tween_textlabels[i], " ");
      const float pivot = 133.333 / (TTL_LAST * 2) * (i * 2 + 1);
      
      float mwid = 0;
      for(int i = 0; i < lines.size(); i++)
        mwid = max(mwid, getTextWidth(lines[i], ticker_text_size));
      mwid += 2;
      
      ranges.push_back(make_pair(pivot - mwid / 2, pivot + mwid / 2)); 
    }
    
    // Second: Traverse all players and update them as necessary.
    for(int player = 0; player < sps_playermode.size(); player++) {
      // Subfirst: See if the player's no longer idle.
      if(sps_playermode[player] == SPS_IDLE) {
        if(keys[player].l.down || keys[player].r.down || keys[player].u.down || keys[player].d.down)
          sps_playermode[player] = SPS_CHOOSING;
      }
      
      // Subsecond: Move the cursor.
      if(sps_playermode[player] == SPS_CHOOSING) {
        Float2 dz = deadzone(keys[player].menu, DEADZONE_CENTER, 0.2) / 2;
        sps_playerpos[player].x += dz.x;
        sps_playerpos[player].y -= dz.y;
        sps_playerpos[player] = clamp(sps_playerpos[player], Float4(0, 90, 133.333, 100));
      }
      
      // Subthird: Do various things depending on the player's current mode
      if(sps_playermode[player] == SPS_IDLE) {
      } else if(sps_playermode[player] == SPS_CHOOSING) {
        bool accept = false;
        if(pms[player].faction) {
          accept = pms[player].genKeystate(keys[player]).accept.push;
        } else {
          for(int j = 0; j < keys[player].keys.size(); j++)
            if(keys[player].keys[j].push)
              accept = true;
        }
        if(accept) {
          for(int j = 0; j < ranges.size(); j++) {
            if(sps_playerpos[player].x == clamp(sps_playerpos[player].x, ranges[j].first, ranges[j].second)) {
              if(sps_ingame[player]) {
                if(j == TTL_DONE) {
                  if(!sps_shopped[player]) {
                    btt_notify = pms[player].faction->faction;
                    btt_frames_left = 180;
                  } else {
                    sps_playermode[player] = SPS_DONE;
                  }
                } else {
                  sps_playermode[player] = SPS_PENDING;
                  sps_queue.push_back(make_pair(player, j));
                }
              } else {
                if(j == TTL_LEAVEJOIN) {
                  sps_playermode[player] = SPS_PENDING;
                  sps_queue.push_back(make_pair(player, j));
                }
                // otherwise we just ignore it
              }
            }
          }
        }
      } else if(sps_playermode[player] == SPS_PENDING) {
        bool cancel = false;
        if(pms[player].faction) {
          cancel = pms[player].genKeystate(keys[player]).cancel.push;
        } else {
          for(int j = 0; j < keys[player].keys.size(); j++)
            if(keys[player].keys[j].push)
              cancel = true;
        }
        if(cancel) {
          sps_playermode[player] = SPS_CHOOSING;
          
          bool found = false;
          for(int j = 0; j < sps_queue.size(); j++) {
            if(sps_queue[j].first == player) {
              CHECK(!found);
              found = true;
              sps_queue.erase(sps_queue.begin() + j);
              j--;
            }
          }
          CHECK(found);
        }
      } else if(sps_playermode[player] == SPS_ACTIVE) {
        // Iterate over items, see if this player is finished
        bool foundrunning = false;
        for(int i = 0; i < slot_count; i++) {
          if(slot[i].pid == player) {
            CHECK(slot[i].type != Slot::EMPTY);
            foundrunning = true;
          }
        }
        if(!foundrunning)
          sps_playermode[player] = SPS_CHOOSING;
      } else if(sps_playermode[player] == SPS_DONE) {
        // Let the player cancel
        CHECK(pms[player].faction);
        if(pms[player].genKeystate(keys[player]).cancel.push)
          sps_playermode[player] = SPS_CHOOSING;
      } else {
        dprintf("Player %d is %d\n", player, sps_playermode[player]);
        CHECK(0);
      }
    }
    
    // Third: Update queues and start new processes
    sort(sps_queue.begin(), sps_queue.end(), QueueSorter());
    // For each item in the queue, try to jam it somewhere.
    while(sps_queue.size()) {
      CHECK(sps_playermode[sps_queue[0].first] == SPS_PENDING);
      const int desired_slots = (sps_queue[0].second == TTL_FULLSHOP ? 1 : 4);
      
      // Do we need to change modes?
      if(slot_count != desired_slots) {
        // See if any slot is still in use.
        bool not_empty = false;
        for(int i = 0; i < slot_count; i++)
          if(slot[i].type != Slot::EMPTY)
            not_empty = true;
        if(not_empty)
          break;  // Ain't gonna happen.
        slot_count = desired_slots;
      }
      
      CHECK(slot_count == desired_slots);
      
      int empty = -1;
      for(int i = 0; i < slot_count; i++) {
        if(slot[i].type == Slot::EMPTY) {
          empty = i;
          break;
        }
      }
      if(empty == -1)
        break;  // No empty slots.
      
      // Okay we've got an empty slot.
      CHECK(slot[empty].type == Slot::EMPTY);
      slot[empty].pid = sps_queue[0].first;
      if(sps_queue[0].second == TTL_LEAVEJOIN && !pms[sps_queue[0].first].faction) {
        slot[empty].type = Slot::CHOOSE;
      } else if(sps_queue[0].second == TTL_LEAVEJOIN && pms[sps_queue[0].first].faction) {
        slot[empty].type = Slot::QUITCONFIRM;
      } else if(sps_queue[0].second == TTL_FULLSHOP) {
        slot[empty].type = Slot::SHOP;
        slot[empty].shop.init(&playerdata[playerid[sps_queue[0].first]], false);
      } else if(sps_queue[0].second == TTL_QUICKSHOP) {
        slot[empty].type = Slot::SHOP;
        slot[empty].shop.init(&playerdata[playerid[sps_queue[0].first]], true);
      } else if(sps_queue[0].second == TTL_SETTINGS) {
        slot[empty].type = Slot::SETTINGS;
      } else {
        CHECK(0);
      }
      
      sps_playermode[sps_queue[0].first] = SPS_ACTIVE;
      sps_queue.erase(sps_queue.begin());
    }
    
    // Fourth: If we're empty, reset to 1 item.
    {
      bool notempty = false;
      for(int i = 0; i < slot_count; i++)
        if(slot[i].type != Slot::EMPTY)
          notempty = true;
      if(!notempty)
        slot_count = 1;
    }
    
    btt_frames_left--;
    if(btt_frames_left <= 0)
      btt_notify = NULL;
    
    // Are we done?
    if(getUnfinishedFactions().size() == 0)
      return true;
    
  } else {
    CHECK(0);
  }
  return false;
}

void PersistentData::render() const {
  smart_ptr<GfxWindow> gfxwpos;
  
  if(mode == TM_SHOP && slot[0].type != Slot::RESULTS) {
    // Draw our framework
    setZoom(Float4(0, 0, 133.333, 100));
    setColor(C::active_text);
    drawLine(Float4(0, divider_ypos, 140, divider_ypos), 0.1);
    drawLine(Float4(0, ticker_ypos, 140, ticker_ypos), 0.1);
    
    // Draw our text descriptions
    setColor(C::inactive_text);
    drawJustifiedText("Next - ", ticker_text_size, Float2(ticker_queue_border, (divider_ypos + ticker_ypos) / 2), TEXT_MIN, TEXT_CENTER);
    drawJustifiedText("- Not ready", ticker_text_size, Float2(133.333 - ticker_waiting_border, (divider_ypos + ticker_ypos) / 2), TEXT_MAX, TEXT_CENTER);
    
    // Draw our text labels
    for(int i = 0; i < TTL_LAST; i++) {
      vector<string> lines = tokenize(tween_textlabels[i], " ");
      const float pivot = 133.333 / (TTL_LAST * 2) * (i * 2 + 1);
      drawJustifiedMultiText(lines, ticker_text_size, Float2(pivot, (ticker_ypos + 100) / 2), TEXT_CENTER, TEXT_CENTER);
    }
    
    // Draw our queues
    {
      bool furtherbuffer = false;
      for(int i = 0; i < sps_queue.size(); i++) {
        Float4 drawpos = Float4(0, 0, ticker_text_size, ticker_text_size) + Float2(getTextWidth("Next - ", ticker_text_size) + ticker_queue_border + (i + furtherbuffer * 0.4) * ticker_text_size * 1.2, (divider_ypos + ticker_ypos) / 2 - ticker_text_size / 2);
        if(i && !furtherbuffer && ((sps_queue[i].second == TTL_FULLSHOP) != (sps_queue[i - 1].second == TTL_FULLSHOP))) {
          setColor(C::gray(0.7));
          drawLine(Float2(drawpos.sx, drawpos.sy), Float2(drawpos.sx, drawpos.ey), 0.1);
          furtherbuffer = true;
          i--;
          continue; // ahahahah so evil
        }
        FactionState *fs = pms[sps_queue[i].first].faction;
        if(fs) {
          setColor(fs->faction->color);
          drawDvec2(fs->faction->icon, drawpos, 10, 0.001);
        } else {
          setColor(C::gray(0.8));
          drawCrosshair(drawpos.midpoint(), ticker_text_size / 2, 0.1);
        }
      }
    }
    
    // Draw our ready-or-not-ready
    {
      vector<const IDBFaction *> nrfactions = getUnfinishedFactions();
      
      for(int i = 0; i < nrfactions.size(); i++) {
        Float4 drawpos = Float4(-ticker_text_size, 0, 0, ticker_text_size) + Float2(133.333 - ticker_waiting_border - getTextWidth("- Not ready", ticker_text_size) - (i + 1) * ticker_text_size * 1.2 + ticker_text_size, (divider_ypos + ticker_ypos) / 2 - ticker_text_size / 2);
        if(nrfactions[i]) {
          setColor(nrfactions[i]->color);
          drawDvec2(nrfactions[i]->icon, drawpos, 10, 0.001);
        } else {
          setColor(C::gray(0.8));
          drawCrosshair(drawpos.midpoint(), ticker_text_size / 2, 0.1);
        }
      }
    }
    
    if(btt_notify) {
      setColor(btt_notify->color);
      drawJustifiedText("Use shop first", ticker_text_size, Float2(133.333 - ticker_waiting_border, ticker_ypos + 0.5), TEXT_MAX, TEXT_MIN);
    }
    
    // Draw our crosshairs
    for(int i = 0; i < sps_playermode.size(); i++) {
      if(sps_playermode[i] == SPS_CHOOSING) {
        if(pms[i].faction)
          setColor(pms[i].faction->faction->color);
        else
          setColor(C::gray(0.8));
        drawCrosshair(Float2(sps_playerpos[i].x, sps_playerpos[i].y), ticker_text_size, 0.1);
        if(pms[i].faction)
          drawDvec2(pms[i].faction->faction->icon, Float4(0, 0, ticker_text_size, ticker_text_size) + sps_playerpos[i] + Float2(ticker_text_size, ticker_text_size) / 10, 10, 0.001);
      }
    }
    
    gfxwpos.reset(new GfxWindow(Float4(0, 0, 133.333, divider_ypos), 1.0));
    
    setZoom(Float4(0, 0, getAspect(), 1.0));
  }
  
  if(slot_count == 1) {
    renderSlot(0);
  } else if(slot_count == 4) {
    setColor(C::active_text);
    drawLine(Float4(0, 0.5, getAspect(), 0.5), 0.001);
    drawLine(Float4(getAspect() / 2, 0, getAspect() / 2, 1), 0.001);
    
    {
      GfxWindow gfxw2(Float4(0, 0, getAspect() / 2, 0.5), 1.0);
      renderSlot(0);
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0, getAspect(), 0.5), 1.0);
      renderSlot(1);
    }
    {
      GfxWindow gfxw2(Float4(0, 0.5, getAspect() / 2, 1), 1.0);
      renderSlot(2);
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0.5, getAspect(), 1), 1.0);
      renderSlot(3);
    }
  } else {
    CHECK(0);
  }
}

void PersistentData::initForShop() {
  mode = TM_SHOP;
  
  sps_shopped.clear();
  sps_shopped.resize(pms.size(), false);
  
  sps_ingame.resize(pms.size());
  for(int i = 0; i < sps_ingame.size(); i++)
    sps_ingame[i] = !!pms[i].faction;
  
  sps_playermode.clear();
  sps_playermode.resize(pms.size(), SPS_IDLE);
  for(int i = 0; i < sps_ingame.size(); i++)
    if(sps_ingame[i])
      sps_playermode[i] = SPS_CHOOSING;
  
  sps_playerpos.clear();
  sps_playerpos.resize(pms.size(), Float2(133.333 / (TTL_LAST * 2) * (TTL_QUICKSHOP * 2 + 1), 95));  // TODO: base this on the constants
}

bool PersistentData::tickSlot(int slotid, const vector<Controller> &keys) {
  CHECK(slotid >= 0 && slotid < 4);
  Slot &slt = slot[slotid];
  if(slt.type == Slot::CHOOSE) {
    StackString stp("Playerchoose");
    CHECK(slt.pid == -1);
    CHECK(keys.size() > 1);

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
      if(readyusers == chosenusers && chosenusers >= 2)
        return true;
    }
  } else if(slt.type == Slot::RESULTS) {
    CHECK(slotid == 0);
    CHECK(slt.pid == -1);
    CHECK(keys.size() > 1);
    vector<Keystates> ki = genKeystates(keys);
    CHECK(slt.type == Slot::RESULTS);
    StackString stp("Results");
    // this is a bit hacky - SHOP mode when slot[0].pid is -1 is the "show results" screen
    for(int i = 0; i < ki.size(); i++) {
      CHECK(SIMUL_WEAPONS == 2);
      if(ki[i].accept.push || ki[i].fire[0].push || ki[i].fire[1].push)
        checked[i] = true;
    }
    if(count(checked.begin(), checked.end(), false) == 0) {
      for(int i = 0; i < playerdata.size(); i++)
        playerdata[i].addCash(lrCash[i]);
      return true;
    }
  } else if(slt.type == Slot::SHOP) {
    StackString stp("Shop");
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    CHECK(keys.size() == 1);
    
    // TODO: this is horrific
    Keystates thesekeys = genKeystates(vector<Controller>(playerdata.size(), keys[0]))[slt.pid];
    
    bool srt = slt.shop.runTick(thesekeys);
    
    if(srt)
      sps_shopped[slt.pid] = true;
    
    return srt;
  } else {
    CHECK(0);
  }
  return false;
}

void PersistentData::renderSlot(int slotid) const {
  CHECK(slotid >= 0 && slotid < 4);
  const Slot &slt = slot[slotid];
  if(slt.type == Slot::CHOOSE) {
    StackString stp("Playerchoose");
    setZoomCenter(0, 0, 1.1);
    setColor(1.0, 1.0, 1.0);
    //drawRect(Float4(-(4./3), -1, (4./3), 1), 0.001);
    for(int i = 0; i < pms.size(); i++) {
      runSettingRender(pms[i]);
    }
    for(int i = 0; i < factions.size(); i++) {
      if(!factions[i].taken) {
        setColor(factions[i].faction->color);
        drawDvec2(factions[i].faction->icon, boxAround(factions[i].compass_location.midpoint(), factions[i].compass_location.y_span() / 2 * 0.9), 50, 0.003);
        //drawRect(factions[i].compass_location, 0.003);
      }
    }
    setColor(1.0, 1.0, 1.0);
    {
      vector<string> txt;
      txt.push_back("Select your faction");
      txt.push_back("icon and configure");
      txt.push_back("your controller");
      drawJustifiedMultiText(txt, 0.05, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
    }
  } else if(slt.type == Slot::SHOP) {
    StackString stp("Shop");
    slt.shop.renderToScreen();
  } else if(slt.type == Slot::RESULTS) {
    StackString stp("Results");
    CHECK(lrCategory.size()); // make sure we *have* results
    setZoom(Float4(0, 0, 800, 600));
    setColor(1.0, 1.0, 1.0);
    drawText("Damage", 30, Float2(20, 20));
    drawText("Kills", 30, Float2(20, 80));
    drawText("Wins", 30, Float2(20, 140));
    drawText("Base", 30, Float2(20, 200));
    drawText("Totals", 30, Float2(20, 320));
    drawMultibar(lrCategory[0], Float4(200, 20, 700, 60));
    drawMultibar(lrCategory[1], Float4(200, 80, 700, 120));
    drawMultibar(lrCategory[2], Float4(200, 140, 700, 180));
    drawMultibar(lrCategory[3], Float4(200, 200, 700, 240));
    drawMultibar(lrPlayer, Float4(200, 320, 700, 360));
    setColor(1.0, 1.0, 1.0);
    drawJustifiedText("Waiting for", 30, Float2(400, 400), TEXT_CENTER, TEXT_MIN);
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
  } else if(slt.type == Slot::EMPTY) {
    setZoomCenter(0, 0, 1);
    setColor(C::gray(0.2));
    drawJustifiedText("zooooom", 0.1, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
  } else {
    CHECK(0);
  }
}

vector<const IDBFaction *> PersistentData::getUnfinishedFactions() const {
  vector<const IDBFaction *> nrfactions;
  for(int i = 0; i < pms.size(); i++) {
    bool ready = false;
    
    if(sps_playermode[i] == SPS_DONE)
      ready = true;
    
    if(!pms[i].faction && sps_playermode[i] == SPS_IDLE)
      ready = true;
    
    if(!pms[i].faction && sps_playermode[i] == SPS_CHOOSING)
      ready = true;
    
    if(!ready)
      nrfactions.push_back(pms[i].faction->faction);
  }
  return nrfactions;
}

vector<Ai *> PersistentData::distillAi(const vector<Ai *> &ai) const {
  CHECK(ai.size() == pms.size());
  vector<Ai *> rv;
  for(int i = 0; i < pms.size(); i++)
    if(pms[i].faction)
      rv.push_back(ai[i]);
  return rv;
}

void PersistentData::ai(const vector<Ai *> &ais) const {
  if(mode == TM_PLAYERCHOOSE) {
    for(int i = 0; i < ais.size(); i++)
      if(ais[i])
        ais[i]->updateCharacterChoice(factions, pms, i);
  } else if(mode == TM_RESULTS) {
    for(int i = 0; i < ais.size(); i++)
      if(ais[i])
        ais[i]->updateWaitingForReport();
  } else if(mode == TM_SHOP) {
    // TODO: this will be complicated
    /*
    if(slot[0].pid == -1) {
      for(int i = 0; i < ais.size(); i++)
        if(ais[i])
          ais[i]->updateWaitingForReport();
    } else {
      CHECK(slot[0].pid >= 0 && slot[0].pid < playerdata.size());
      slot[0].shop.ai(distillAi(ais)[slot[0].pid]);
      for(int i = 0; i < ais.size(); i++)
        if(ais[i] && i != slot[0].pid)
          ais[i]->updateIdle();
    }*/
  } else {
    CHECK(0);
  }
}

vector<Keystates> PersistentData::genKeystates(const vector<Controller> &keys) const {
  vector<Keystates> kst(playerdata.size());
  int ct = 0;
  set<int> kstd;
  for(int i = 0; i < pms.size(); i++) {
    if(pms[i].faction) {
      kst[playerid[i]] = pms[i].genKeystate(keys[i]);
      ct++;
      kstd.insert(playerid[i]);
    }
  }
  CHECK(kstd.size() == ct);
  CHECK(ct == kst.size());
  return kst;
}

DECLARE_int(startingCash);  // defaults to 1000 atm

void PersistentData::divvyCash(float firepowerSpent) {
  shopcycles++;
  
  checked.clear();
  checked.resize(playerdata.size());
  
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
  dprintf("%d, %f\n", shopcycles, firepowerSpent);
  long double totalReturn = (75 / 1000 * FLAGS_startingCash) * powl(1.08, roundsbetweenshop * shopcycles) * playerdata.size() * roundsbetweenshop + firepowerSpent * 0.8;
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
  
  mode = TM_RESULTS;
  slot_count = 1;
  slot[0].type = Slot::RESULTS;
  slot[0].pid = -1;
}

void PersistentData::drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const {
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

DEFINE_int(debugControllers, 0, "Number of controllers to set to debug defaults");

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

PersistentData::PersistentData(int playercount, int in_roundsbetweenshop) {
  roundsbetweenshop = in_roundsbetweenshop;
  
  pms.clear();
  pms.resize(playercount, PlayerMenuState(Float2(0, 0)));
  playerid.resize(playercount, -1);
  
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
  
  {
    pair<Float4, vector<Float2> > factcents = getFactionCenters(factions.size());
    
    vector<Float2> centgrays;
    // technically with both of these vectors I should be using a custom comparator but I'm really fucking lazy
    vector<pair<float, Float2> > centangs;
    for(int i = 0; i < factcents.second.size(); i++) {
      if(factcents.second[i].y != 0) {
        centangs.push_back(make_pair(getAngle(factcents.second[i]), factcents.second[i]));
      } else {
        centgrays.push_back(factcents.second[i]);
      }
    }
    sort(centangs.begin(), centangs.end());
    
    vector<int> factgrays;
    vector<pair<float, int> > facthues;
    for(int i = 0; i < factions.size(); i++) {
      if(factions[i].faction->color.getHue() != -1) {
        facthues.push_back(make_pair(factions[i].faction->color.getHue(), i));
      } else {
        factgrays.push_back(i);
      }
    }
    sort(facthues.begin(), facthues.end());
    
    CHECK(centgrays.size() == factgrays.size());
    CHECK(centangs.size() == facthues.size());  // this is kind of flimsy
    
    for(int i = 0; i < facthues.size(); i++)
      factions[facthues[i].second].compass_location = factcents.first + centangs[i].second;
    for(int i = 0; i < factgrays.size(); i++)
      factions[factgrays[i]].compass_location = factcents.first + centgrays[i];
  }
  
  mode = TM_PLAYERCHOOSE;
  
  shopcycles = 0;
  
  CHECK(FLAGS_debugControllers >= 0 && FLAGS_debugControllers <= 2);
  CHECK(factions.size() >= FLAGS_debugControllers);

  if(FLAGS_debugControllers >= 1) {
    CHECK(pms.size() >= 1); // better be
    pms[0].faction = &factions[0];
    factions[0].taken = true;
    pms[0].settingmode = SETTING_READY;
    pms[0].choicemode = CHOICE_IDLE;
    pms[0].buttons[BUTTON_FIRE1] = 4;
    pms[0].buttons[BUTTON_FIRE2] = 8;
    pms[0].buttons[BUTTON_SWITCH1] = 5;
    pms[0].buttons[BUTTON_SWITCH2] = 9;
    pms[0].buttons[BUTTON_ACCEPT] = 4;
    pms[0].buttons[BUTTON_CANCEL] = 8;
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
    pms[1].buttons[BUTTON_FIRE1] = 2;
    pms[1].buttons[BUTTON_FIRE2] = 5;
    pms[1].buttons[BUTTON_SWITCH1] = 1;
    pms[1].buttons[BUTTON_SWITCH2] = 5;
    pms[1].buttons[BUTTON_ACCEPT] = 2;
    pms[1].buttons[BUTTON_CANCEL] = 5;
    CHECK(pms[1].buttons.size() == 6);
    pms[1].axes[0] = 0;
    pms[1].axes[1] = 1;
    pms[1].axes_invert[0] = false;
    pms[1].axes_invert[1] = false;
    pms[1].setting_axistype = KSAX_ABSOLUTE;
    pms[1].fireHeld = 0;
  }
  
  CHECK(sizeof(slot) / sizeof(*slot) == 4);
  for(int i = 0; i < 4; i++) {
    slot[i].type = Slot::EMPTY;
    slot[i].pid = -1;
  }
  
  slot[0].type = Slot::CHOOSE;
  slot[0].pid = -1;
  
  slot_count = 1;
  
  btt_notify = NULL;
  btt_frames_left = 0;
}
