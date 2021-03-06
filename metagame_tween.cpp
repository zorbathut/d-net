
#include "metagame_tween.h"

#include "adler32_util.h"
#include "ai.h"
#include "dumper_registry.h"
#include "game_projectile.h"
#include "game_tank.h"
#include "gfx.h"
#include "parse.h"
#include "perfbar.h"

#include <numeric>

using namespace std;

DEFINE_bool(hideAiShopping, true, "Allow skipping bits when waiting for the AI to buy stuff");

const float divider_ypos = 87;
const float ticker_ypos = 90;

const float ticker_text_size = 2;
const float ticker_queue_border = 1;
const float ticker_waiting_border = 1;

const float value_ratios[4] = { 1.0, 1.0, 0.6, 3.0 };

bool PersistentData::isPlayerChoose() const {
  return mode == TM_PLAYERCHOOSE;
}

vector<Player> &PersistentData::players() {
  return playerdata;
}
const vector<Player> &PersistentData::players() const {
  return playerdata;
}

const char * const tween_textlabels[] = {"Quit and end game", "Leave/join game", "Leave game", "Join game", "Settings", "Shop", "Next round"};
enum { TTL_END, TTL_LEAVEJOIN, TTL_LEAVE, TTL_JOIN, TTL_SETTINGS, TTL_SHOP, TTL_DONE, TTL_LAST }; 

class QueueSorter {
  const vector<PlayerMenuState> *pms;
  
  int precedence(pair<int, int> item) const {
    //if(item.second == TTL_FULLSHOP)
      //return 2;
    // Prioritize leaving or joining the game
    if(item.second == TTL_LEAVEJOIN || item.second == TTL_JOIN || item.second == TTL_LEAVE)
      return 0;
    return 1;
  }
  
public:
  bool operator()(const pair<int, int> &lhs, const pair<int, int> &rhs) const {
    return precedence(lhs) < precedence(rhs);
  }
  
  QueueSorter(const vector<PlayerMenuState> &pmsv) {
    pms = &pmsv;
    CHECK(pms);
  }
};

PersistentData::PDRTR PersistentData::tick(const vector<Controller> &keys, const InputSnag &is) {
  StackString sps("Persistentdata tick");
  PerfStack pst(PBC::persistent);
  
  CHECK(keys.size() == pms.size());
  CHECK(humans.size() == keys.size());
  
  for(int i = 0; i < keys.size(); i++)
    CHECK(keys[i].human == humans[i]);

  // Update our sound info
  for(int i = 0; i < sps_soundtimeout.size(); i++)
    sps_soundtimeout[i]--;
  
  // First: traverse and empty.
  for(int i = 0; i < slot_count; i++) {
    if(slot[i].type == Slot::EMPTY)
      continue;
    
    smart_ptr<AudioToner> at;
    smart_ptr<AudioShifter> as;
    if(slot_count == 1) {
    } else if(slot_count == 4) {
      const double octdiff = 1.25;
      at.reset(new AudioToner(pow(octdiff, 3 - i) / pow(octdiff, 1.5)));
      
      if(i % 2 == 0) {
        as.reset(new AudioShifter(1.0, 0));
      } else {
        as.reset(new AudioShifter(0, 1.0));
      }
    } else {
      CHECK(0);
    }
    
    bool clear;
    if(slot[i].pid == -1) {
      clear = tickSlot(i, keys, is);
    } else {
      clear = tickSlot(i, vector<Controller>(1, keys[slot[i].pid]), is);
    }
    if(clear) {
      slot[i].type = Slot::EMPTY;
      slot[i].pid = -1;
    }
  }
  
  // Next: deal with empty slots depending on our mode.
  
  if(mode == TM_RESULTS) {
    CHECK(slot_count == 1);
    if(slot[0].type == Slot::EMPTY) {
      if(shopcycles * roundsbetweenshop >= rounds_until_end) {
        enterGameEnd();
      } else {
        mode = TM_SHOP;
        reset();
      }
    }
  } else if(mode == TM_GAMEEND) {
    CHECK(slot_count == 1);
    if(slot[0].type == Slot::EMPTY) {
      return PDRTR_EXIT;
    }
  } else if(mode == TM_SHOP || mode == TM_PLAYERCHOOSE) {
    StackString sps(StringPrintf("Stdtween"));
    // Various complications and such!
    
    // First used to be calculating ranges. It isn't anymore, they're precalculated (yes it was an actual speed problem)
    
    // Second: Traverse all players and update them as necessary.
    for(int player = 0; player < sps_playermode.size(); player++) {
      StackString sps(StringPrintf("Traversing player %d", player));
      // Subfirst: See if the player's no longer idle.
      if(sps_playermode[player] == SPS_IDLE) {
        if(keys[player].l.down || keys[player].r.down || keys[player].u.down || keys[player].d.down)
          sps_playermode[player] = SPS_CHOOSING;
      }
      
      // Subsecond: Move the cursor.
      if(sps_playermode[player] == SPS_CHOOSING) {
        Coord2 dz = deadzone(keys[player].menu, DEADZONE_CENTER, 0.2) / 1.5;
        
        bool wasin = false;
        for(int j = 0; j < ranges.size(); j++)
          if(sps_playerpos[player].x == clamp(sps_playerpos[player].x, ranges[j].second.first, ranges[j].second.second))
            wasin = true;
        
        sps_playerpos[player].x += dz.x;
        sps_playerpos[player].y -= dz.y;
        sps_playerpos[player] = clamp(sps_playerpos[player], Coord4(0, 90, 100, 100));
        
        if(!wasin) {
          bool isin = false;
          for(int j = 0; j < ranges.size(); j++)
            if(sps_playerpos[player].x == clamp(sps_playerpos[player].x, ranges[j].second.first, ranges[j].second.second))
              isin = true;
          if(isin)
            queueSound(S::cursorover);
        }
        
        CHECK(pms.size() == sps_playermode.size());
      }
      
      // Subthird: Do various things depending on the player's current mode
      if(sps_playermode[player] == SPS_IDLE) {
      } else if(sps_playermode[player] == SPS_CHOOSING) {
        
        bool accept = false;
        if(pms[player].faction) {
          accept = pms[player].genKeystate(keys[player]).accept_or_fire.push;
        } else {
          for(int j = 0; j < keys[player].keys.size(); j++)
            if(keys[player].keys[j].push)
              accept = true;
        }
        if(accept) {
          for(int j = 0; j < ranges.size(); j++) {
            if(sps_playerpos[player].x == clamp(sps_playerpos[player].x, ranges[j].second.first, ranges[j].second.second)) {
              if(pms[player].faction) {
                if(ranges[j].first == TTL_DONE) {
                  if(!sps_shopped[player] && mode == TM_SHOP) {
                    attemptQueueSound(player, S::error);
                    btt_notify = pms[player].faction->faction;
                    btt_frames_left = 180;
                  } else {
                    attemptQueueSound(player, S::choose);
                    sps_playermode[player] = SPS_DONE;
                  }
                } else if(ranges[j].first == TTL_END) {
                  attemptQueueSound(player, S::choose);
                  sps_playermode[player] = SPS_END;
                } else if(ranges[j].first == TTL_JOIN) {
                  attemptQueueSound(player, S::error);
                } else {
                  queueSound(S::choose);
                  sps_playermode[player] = SPS_PENDING;
                  sps_queue.push_back(make_pair(player, ranges[j].first));
                }
              } else {
                if(ranges[j].first == TTL_LEAVEJOIN || ranges[j].first == TTL_JOIN) {
                  queueSound(S::choose);
                  sps_playermode[player] = SPS_PENDING;
                  sps_queue.push_back(make_pair(player, ranges[j].first));
                } else {
                  attemptQueueSound(player, S::error);
                }
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
          
          queueSound(S::choose);
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
        if(!foundrunning) {
          if(playerid[player] == -1) {
            sps_playermode[player] = SPS_IDLE;
          } else {
            sps_playermode[player] = SPS_CHOOSING;
          }
        }
      } else if(sps_playermode[player] == SPS_DONE || sps_playermode[player] == SPS_END) {
        // Let the player cancel
        CHECK(pms[player].faction);
        if(pms[player].genKeystate(keys[player]).cancel.push) {
          attemptQueueSound(player, S::choose);
          sps_playermode[player] = SPS_CHOOSING;
        }
      } else {
        dprintf("Player %d is %d\n", player, sps_playermode[player]);
        CHECK(0);
      }
    }
    
    // Third: Update queues and start new processes
    {
      StackString sps(StringPrintf("Queueing"));
      sort(sps_queue.begin(), sps_queue.end(), QueueSorter(pms));
      // For each item in the queue, try to jam it somewhere.
      while(sps_queue.size()) {
        CHECK(sps_playermode[sps_queue[0].first] == SPS_PENDING);
        bool shop_is_quickshop = (getHumanCount() > 1 || getAiCount() == 0 || !humans[sps_queue[0].first]);  
        const int desired_slots = ((sps_queue[0].second == TTL_SHOP && !shop_is_quickshop) ? 1 : 4);
        
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
          sps_quitconfirm[sps_queue[0].first] = 0;
        } else if(sps_queue[0].second == TTL_LEAVE) {
          CHECK(pms[sps_queue[0].first].faction);
          slot[empty].type = Slot::QUITCONFIRM;
        } else if(sps_queue[0].second == TTL_JOIN) {
          dprintf("Putting this beast back in the choose\n");
          CHECK(!pms[sps_queue[0].first].faction);
          slot[empty].type = Slot::CHOOSE;
        } else if(sps_queue[0].second == TTL_SHOP) {
          slot[empty].type = Slot::SHOP;
          slot[empty].shop.init(shop_is_quickshop, &playerdata[playerid[slot[empty].pid]], getExpectedPlayercount(), highestPlayerCash, getScreenAspect() * 100 / divider_ypos);
        } else if(sps_queue[0].second == TTL_SETTINGS) {
          slot[empty].type = Slot::SETTINGS;
        } else {
          CHECK(0);
        }
        
        sps_playermode[sps_queue[0].first] = SPS_ACTIVE;
        sps_queue.erase(sps_queue.begin());
      }
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
    if(getUnfinishedFactions().size() == 0 && playerdata.size() >= 1) {
      // Okay, we're done. Things are a bit complicated from here.
      // If humans exist, we must have at least one complete, and the humans have authority.
      // If humans don't exist, then we can rely on the AIs.
      
      bool havehumans = false;
      CHECK(keys.size() == sps_playermode.size());
      for(int i = 0; i < sps_playermode.size(); i++)
        if(keys[i].human)
          havehumans = true;
      
      int done = 0;
      int end = 0;
      for(int i = 0; i < sps_playermode.size(); i++) {
        if(havehumans && !keys[i].human)
          continue;
        
        if(sps_playermode[i] == SPS_DONE) {
          done++;
        } else if(sps_playermode[i] == SPS_END) {
          end++;
        }
      }
      
      if(!done && !end) {
      } else if(end >= done) {
        enterGameEnd();
      } else if(playerdata.size() >= 2 && getExpectedPlayercount() == playerdata.size()) {
        for(int i = 0; i < sps_playermode.size(); i++)
          if(sps_playermode[i] == SPS_END)
            destroyPlayer(i);
        mode = TM_SHOP; // if we're in PLAYERCHOOSE mode, then reset() won't be able to get the shop item positions for existing units
        reset();
        return PDRTR_PLAY;
      }
    }
    
  } else {
    CHECK(0);
  }
  return PDRTR_CONTINUE;
}

void PersistentData::render(const InputSnag &is) const {
  smart_ptr<GfxWindow> gfxwpos;
  
  if(isWaitingOnAi() && FLAGS_hideAiShopping) {
    setZoomVertical(0, 0, 100);
    setColor(C::inactive_text);
    
    drawJustifiedText("Please wait, the AIs are busy.", 4, Float2(getZoom().ex / 2, 20), TEXT_CENTER, TEXT_CENTER);
    
    CHECK(humans.size());
    int done = 0;
    
    for(int i = 0; i < pms.size(); i++)
      if(sps_playermode[i] == SPS_DONE && !humans[i])
        done++;
    
    drawJustifiedText(StringPrintf("%d done out of %d", done, getAiCount()), 4, Float2(getZoom().ex / 2, 80), TEXT_CENTER, TEXT_CENTER);
    
    setColor(C::active_text);
    
    vector<string> opts;
    opts.push_back("Buying every weapon in existence . . .");
    opts.push_back("Driving away while firing . . .");
    opts.push_back("Failing to upgrade tanks . . .");
    opts.push_back("Fuming at the interface . . .");
    opts.push_back("Getting stuck in a corner . . .");
    opts.push_back("Head-butting the opposition . . .");
    opts.push_back("Playing bumper-tanks . . .");
    opts.push_back("Purchasing equipment . . .");
    opts.push_back("Racing into walls . . .");
    opts.push_back("Running in circles . . .");
    opts.push_back("Shooting a wall . . .");
    opts.push_back("Smashing into things . . .");
    
    drawJustifiedText(opts[unsync().choose(opts.size())], 3, Float2(getZoom().ex / 2, 50), TEXT_CENTER, TEXT_CENTER);
    
    return;
  }
    
  if(slot[0].type != Slot::RESULTS && slot[0].type != Slot::GAMEEND) {
    setZoomVertical(0, 0, 100);
    gfxwpos.reset(new GfxWindow(Float4(0, 0, getZoom().ex, divider_ypos), 1.0));
    setZoomVertical(0, 0, 1);
    
    vector<string> text;
    if(shopcycles == 0) {
      text.push_back("Choose \"join game\" to add more players.");
      if(!getAiCount())
        text.push_back("At least two players are needed.");
      text.push_back("");
      text.push_back("");
      text.push_back("Right keyboard player: ");
      text.push_back("Use arrow keys for movement and O to choose an option.");
      text.push_back("");
      text.push_back("Left keyboard player:");
      text.push_back("Use WASD for movement and F to choose an option.");
      text.push_back("");
      text.push_back("");
    }
    
    text.push_back("Enter the shop to purchase new, more powerful ammunition,");
    text.push_back("tanks, tank upgrades, and more.");
    text.push_back("");
    text.push_back("");
    
    text.push_back("Choose \"Settings\" to modify your controller settings.");
    text.push_back("");
    text.push_back("");
    
    /*
    if(mode == TM_SHOP) {
      text.push_back("\"Quick shop\" lets four people buy things at once.");
      text.push_back("\"Full shop\" gives weapon and upgrade demonstrations.");
      text.push_back("");
      text.push_back("");
    }*/
    
    text.push_back("Choose \"Next round\" when ready to play.");
    
    if(shopcycles != 0) {
      text.push_back("");
      text.push_back("");
      text.push_back("Press escape to quit the game.");
    }
    
    setColor(C::inactive_text * 0.5);
    drawJustifiedMultiText(text, 0.03, getZoom().midpoint(), TEXT_CENTER, TEXT_CENTER);
    
    if(getUnfinishedFactions().size() == 0 && playerdata.size() < 2 && getExpectedPlayercount() == 1 && getHumanCount()) {
      
      vector<string> txt;
      txt.push_back("Devastation Net is fundamentally a multiplayer game.");
      txt.push_back("");
      txt.push_back("To play, you either need two players,");
      txt.push_back("or one or more AI players. If you want");
      txt.push_back("to add AI players, you currently have to");
      txt.push_back("restart the game.");
      txt.push_back("");
      txt.push_back("Hit Escape, choose \"End Game\",");
      txt.push_back("and create a new game with AI players.");
      txt.push_back("");
      txt.push_back("Alternatively, if this is your first time playing,");
      txt.push_back("you may want to try the Instant Action option.");
      
      drawJustifiedTextBox(txt, 0.03, getZoom().midpoint(), TEXT_CENTER, TEXT_CENTER, C::active_text, C::box_border);
    }
  }
  
  setZoomVertical(0, 0, 1.0);
  
  if(slot_count == 1) {
    renderSlot(0, is);
  } else if(slot_count == 4) {
    {
      GfxWindow gfxw2(Float4(0, 0, getAspect() / 2, 0.5), 1.0);
      renderSlot(0, is);
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0, getAspect(), 0.5), 1.0);
      renderSlot(1, is);
    }
    {
      GfxWindow gfxw2(Float4(0, 0.5, getAspect() / 2, 1), 1.0);
      renderSlot(2, is);
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0.5, getAspect(), 1), 1.0);
      renderSlot(3, is);
    }
    
    setColor(C::inactive_text);
    
    if(slot[0].type != Slot::EMPTY || slot[1].type != Slot::EMPTY)
      drawLine(Float4(getAspect() / 2, 0, getAspect() / 2, 0.5), 0.001);
    
    if(slot[2].type != Slot::EMPTY || slot[3].type != Slot::EMPTY)
      drawLine(Float4(getAspect() / 2, 0.5, getAspect() / 2, 1), 0.001);
    
    if(slot[0].type != Slot::EMPTY || slot[2].type != Slot::EMPTY)
      drawLine(Float4(0, 0.5, getAspect() / 2, 0.5), 0.001);
    
    if(slot[1].type != Slot::EMPTY || slot[3].type != Slot::EMPTY)
      drawLine(Float4(getAspect() / 2, 0.5, getAspect(), 0.5), 0.001);
  } else {
    CHECK(0);
  }
  
  gfxwpos.reset();
  
  if(slot[0].type != Slot::RESULTS && slot[0].type != Slot::GAMEEND) {
    // Draw our framework
    setZoomVertical(0, 0, 100);
    setColor(C::active_text);
    drawLine(Float4(0, divider_ypos, getZoom().ex, divider_ypos), 0.1);
    drawLine(Float4(0, ticker_ypos, getZoom().ex, ticker_ypos), 0.1);
    
    // Draw our text descriptions
    setColor(C::inactive_text);
    drawJustifiedText("Next - ", ticker_text_size, Float2(ticker_queue_border, (divider_ypos + ticker_ypos) / 2), TEXT_MIN, TEXT_CENTER);
    drawJustifiedText("- Not ready", ticker_text_size, Float2(getZoom().ex - ticker_waiting_border, (divider_ypos + ticker_ypos) / 2), TEXT_MAX, TEXT_CENTER);
    
    // Draw our text labels
    {
      for(int i = 0; i < ranges.size(); i++) {
        vector<string> lines = tokenize(tween_textlabels[ranges[i].first], " ");
        if(lines.size() == 4) {
          lines[0] += " " + lines[1];
          lines[1] = lines[2] + " " + lines[3];
          lines.erase(lines.begin() + 2, lines.end());
        }
        drawJustifiedMultiText(lines, ticker_text_size, Coord2((ranges[i].second.first + ranges[i].second.second) / 2 / 100 * getZoom().ex, (ticker_ypos + 100) / 2), TEXT_CENTER, TEXT_CENTER);
      }
    }
    
    // Draw our queues
    {
      //bool furtherbuffer = false;
      for(int i = 0; i < sps_queue.size(); i++) {
        Float4 drawpos = Float4(0, 0, ticker_text_size, ticker_text_size) + Float2(getTextWidth("Next - ", ticker_text_size) + ticker_queue_border + (i /*+ furtherbuffer * 0.4*/) * ticker_text_size * 1.2, (divider_ypos + ticker_ypos) / 2 - ticker_text_size / 2);
        /*if(i && !furtherbuffer && ((sps_queue[i].second == TTL_FULLSHOP) != (sps_queue[i - 1].second == TTL_FULLSHOP))) {
          setColor(C::gray(0.7));
          drawLine(Float2(drawpos.sx, drawpos.sy), Float2(drawpos.sx, drawpos.ey), 0.1);
          furtherbuffer = true;
          i--;
          continue; // ahahahah so evil
        }*/
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
      vector<const IDBFaction *> nrfactions = getUnfinishedHumanFactions();
      
      for(int i = 0; i < nrfactions.size(); i++) {
        Float4 drawpos = Float4(-ticker_text_size, 0, 0, ticker_text_size) + Float2(getZoom().ex - ticker_waiting_border - getTextWidth("- Not ready", ticker_text_size) - (i + 1) * ticker_text_size * 1.2 + ticker_text_size, (divider_ypos + ticker_ypos) / 2 - ticker_text_size / 2);
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
      drawJustifiedText("Use shop first", ticker_text_size, Float2(getZoom().ex - ticker_waiting_border, ticker_ypos + 0.5), TEXT_MAX, TEXT_MIN);
    }
    
    // Draw our crosshairs
    for(int i = 0; i < sps_playermode.size(); i++) {
      if(sps_playermode[i] == SPS_CHOOSING) {
        if(pms[i].faction)
          setColor(pms[i].faction->faction->color);
        else
          setColor(C::gray(0.8));
        Coord2 rpp = sps_playerpos[i];
        rpp.x = rpp.x / 100 * getZoom().ex;
        drawCrosshair(rpp, ticker_text_size, 0.1);
        if(pms[i].faction)
          drawDvec2(pms[i].faction->faction->icon, Coord4(0, 0, ticker_text_size, ticker_text_size) + rpp + Coord2(ticker_text_size, ticker_text_size) / 10, 10, 0.001);
      }
    }
  }
}

void adler(Adler32 *adl, const PersistentData::Slot &slt) {
  adler(adl, slt.type);
  adler(adl, slt.pid);
  slt.shop.checksum(adl);
}

void PersistentData::checksum(Adler32 *adl) const {
  adler(adl, mode);
  adler(adl, playerid);
  adler(adl, factions);
  adler(adl, faction_mode);
  adler(adl, lrCash);
  adler(adl, lrBaseCash);
  adler(adl, checked);
  adler(adl, baseStartingCash);
  adler(adl, multiplePerRound);
  adler(adl, newPlayerStartingCash);
  adler(adl, highestPlayerCash);
  adler_array(adl, slot);
  adler(adl, roundsbetweenshop);
  adler(adl, shopcycles);
  adler(adl, sps_shopped);
  adler(adl, sps_playermode);
  adler(adl, sps_quitconfirm);
  adler(adl, sps_playerpos);
  adler(adl, sps_soundtimeout);
  adler(adl, sps_queue);
  adler(adl, btt_notify);
  adler(adl, btt_frames_left);
  adler(adl, playerdata);
  adler(adl, pms);
}

void PersistentData::reset() {
  sps_shopped.clear();
  sps_shopped.resize(pms.size(), false);
  
  sps_playermode.clear();
  sps_playermode.resize(pms.size(), SPS_IDLE);
  for(int i = 0; i < sps_playermode.size(); i++)
  if(humans[i] && playerid[i] != -1)
    sps_playermode[i] = SPS_CHOOSING;
  
  resetRanges();
  
  sps_playerpos.clear();
  for(int i = 0; i < pms.size(); i++) {
    if(playerid[i] != -1) {
      sps_playerpos.push_back(targetCoords(TTL_SHOP));
    } else if(mode == TM_PLAYERCHOOSE) {
      sps_playerpos.push_back(targetCoords(TTL_JOIN));
    } else {
      sps_playerpos.push_back(targetCoords(TTL_LEAVEJOIN));
    }
  }
}

bool PersistentData::tickSlot(int slotid, const vector<Controller> &keys, const InputSnag &is) {
  CHECK(slotid >= 0 && slotid < 4);
  Slot &slt = slot[slotid];
  if(slt.type == Slot::CHOOSE) {
    StackString stp("CHOOSE");
    CHECK(slt.pid != -1);
    CHECK(keys.size() == 1);

    bool cancel = runSettingTick(keys[0], &pms[slt.pid], factions, is.getcc(slt.pid));

    if(pms[slt.pid].faction) {
      playerid[slt.pid] = playerdata.size();
      playerdata.push_back(Player(pms[slt.pid].faction->faction, faction_mode, newPlayerStartingCash));
      playerdata.back().total_damageDone = newPlayerDamageDone;
      playerdata.back().total_kills = newPlayerKills;
      playerdata.back().total_wins = newPlayerWins;
      slot[slotid].type = Slot::SETTINGS;
    }
    
    if(cancel) {
      return true;
    }
  } else if(slt.type == Slot::RESULTS || slt.type == Slot::GAMEEND) {
    CHECK(slotid == 0);
    CHECK(slt.pid == -1);
    CHECK(keys.size() > 1);
    vector<Keystates> ki = genKeystates(keys);
    StackString stp("Results");
    for(int i = 0; i < ki.size(); i++) {
      bool pushed = false;
      for(int j = 0; j < SIMUL_WEAPONS; j++)
        if(ki[i].fire[j].push)
          pushed = true;
      if(ki[i].accept.push || pushed) {
        checked[i] = !checked[i];
        //queueSound(S::choose);
      }
    }
    if(count(checked.begin(), checked.end(), false) == 0) {
      if(slt.type == Slot::RESULTS)
        for(int i = 0; i < playerdata.size(); i++)
          playerdata[i].addCash(lrCash[i]);
      return true;
    }
  } else if(slt.type == Slot::SHOP) {
    StackString stp("Shop");
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    CHECK(keys.size() == 1);
    
    bool srt = slt.shop.runTick(pms[slt.pid].genKeystate(keys[0]), &playerdata[playerid[slt.pid]], getExpectedPlayercount());
    
    if(srt)
      sps_shopped[slt.pid] = true;
    
    return srt;
  } else if(slt.type == Slot::SETTINGS) {
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    CHECK(keys.size() == 1);
    return runSettingTick(keys[0], &pms[slt.pid], factions, is.getcc(slt.pid));
  } else if(slt.type == Slot::QUITCONFIRM) {
    Keystates thesekeys = pms[slt.pid].genKeystate(keys[0]);
    
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    CHECK(keys.size() == 1);
    if(thesekeys.u.repeat) {
      queueSound(S::select);
      sps_quitconfirm[slt.pid]--;
    }
    if(thesekeys.d.repeat) {
      queueSound(S::select);
      sps_quitconfirm[slt.pid]++;
    }
    
    sps_quitconfirm[slt.pid] = modurot(sps_quitconfirm[slt.pid], 5);
    
    if(thesekeys.cancel.push) {
      queueSound(S::choose);
      return true;
    }
    
    if(thesekeys.accept.push) {
      if(sps_quitconfirm[slt.pid] == 3) {
        destroyPlayer(slt.pid);
        queueSound(S::accept);
      } else {
        queueSound(S::choose);
      }
      return true;
    }
  } else {
    CHECK(0);
  }
  return false;
}

class AdjustSorter {
public:
  bool operator()(const pair<pair<string, bool>, vector<string> > &lhs, const pair<pair<string, bool>, vector<string> > &rhs) {
    if(lhs.first.second != rhs.first.second)
      return lhs.first.second > rhs.first.second;
    if(atoi(lhs.first.first.c_str()) != atoi(rhs.first.first.c_str()))
      return atoi(lhs.first.first.c_str()) > atoi(rhs.first.first.c_str());
    return lhs.second < rhs.second;
  }
};

void PersistentData::renderSlot(int slotid, const InputSnag &is) const {
  CHECK(slotid >= 0 && slotid < 4);
  const Slot &slt = slot[slotid];
  
  if(slt.type == Slot::EMPTY)
    return;
  
  drawSolid(getZoom());
  
  if(slt.type == Slot::CHOOSE) {
    StackString stp("choose");
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    setZoomVertical(0, 0, 1);
    
    const bool ignore_modifiers = (faction_mode == 0);
    
    const float div_x = 0.6;
    
    setColor(C::active_text);
    drawLine(0.05, 0.5, div_x, 0.5, 0.003);
    drawLine(div_x, 0.05, div_x, 0.95, 0.003);
    
    const int fid = pms[slt.pid].current_faction_over;
    const int fid_duration = pms[slt.pid].current_faction_over_duration;
    float compass_opacity;
    {
      if(fid == -1 || ignore_modifiers) {
        compass_opacity = 1.0;
      } else if(fid_duration < FPS * 3) {
        compass_opacity = 1.0;
      } else if(fid_duration < FPS * 5) {
        compass_opacity = float(FPS * 2 - (fid_duration - FPS * 3)) / (FPS * 2);
      } else {
        compass_opacity = 0.0;
      }
    }
    float logo_opacity = 1.0 - compass_opacity;
    
    {
      GfxWindow gfxw(Float4(0, 0.5, div_x, 1), compass_opacity);
      setZoomCenter(0, 0, 1.0);
      for(int i = 0; i < factions.size(); i++) {
        if(!factions[i].taken) {
          setColor(factions[i].faction->color);
          drawDvec2(factions[i].faction->icon, boxAround(factions[i].compass_location.midpoint(), factions[i].compass_location.span_y() / 2 * 0.9), 50, 0.01);
        }
      }
      setColor(1.0, 1.0, 1.0);

      const float comouter = 0.1;
      const float cominner = 0.03;
      const float comthick = 0.01;
      drawLine(pms[slt.pid].compasspos.x, pms[slt.pid].compasspos.y - comouter, pms[slt.pid].compasspos.x, pms[slt.pid].compasspos.y - cominner, comthick);
      drawLine(pms[slt.pid].compasspos.x, pms[slt.pid].compasspos.y + comouter, pms[slt.pid].compasspos.x, pms[slt.pid].compasspos.y + cominner, comthick);
      drawLine(pms[slt.pid].compasspos.x - comouter, pms[slt.pid].compasspos.y, pms[slt.pid].compasspos.x - cominner, pms[slt.pid].compasspos.y, comthick);
      drawLine(pms[slt.pid].compasspos.x + comouter, pms[slt.pid].compasspos.y, pms[slt.pid].compasspos.x + cominner, pms[slt.pid].compasspos.y, comthick);
    }
    
    if(fid == -1) {
      setColor(C::inactive_text);
      GfxWindow gfxw(Float4(div_x, 0, getAspect(), 1.0), 1.0);
      setZoomVertical(0, 0, 1);
      vector<string> steer;
      steer.push_back("Move the cursor over a");
      steer.push_back("faction icon for information");
      steer.push_back("");
      steer.push_back("Press " + is.getcc(slt.pid).active_button + " to");
      steer.push_back("choose that faction");
      drawJustifiedMultiText(steer, 0.04, getZoom().midpoint(), TEXT_CENTER, TEXT_CENTER);
    } else {
      {
        smart_ptr<GfxWindow> gfxw;
        if(ignore_modifiers) {
          gfxw.reset(new GfxWindow(Float4(0, 0, div_x, 0.5), 1.0));
        } else {
          gfxw.reset(new GfxWindow(Float4(0, 0.5, div_x, 1.0), logo_opacity));
        }
        setZoomCenter(0, 0, 1.0);
        setColor(factions[fid].faction->color);
        drawDvec2(factions[fid].faction->icon, contract(getZoom(), 0.1), 50, 0.02);
      }
      if(factions[fid].faction->text) {
        setColor(C::inactive_text);
        GfxWindow gfxw(Float4(div_x, 0, getAspect(), 1.0), 1.0);
        setZoomVertical(0, 0, 1);
        if(ignore_modifiers) {
          vector<string> tok = tokenize(*factions[fid].faction->text, "\n");
          CHECK(tok.size() == 2);
          drawJustifiedParagraphedText(tok[0], 0.04, contract(getZoom(), 0.02).xs(), getZoom().midpoint().y, TEXT_CENTER);
        } else {
          drawJustifiedParagraphedText(*factions[fid].faction->text, 0.04, contract(getZoom(), 0.02).xs(), getZoom().midpoint().y, TEXT_CENTER);
        }
      }
      if(!ignore_modifiers) {
        GfxWindow gfxw(Float4(0, 0, div_x, 0.5), 1.0);
        int lines_needed = 9;
        const float tweensize = 0.6;
        setZoomVertical(0, 0, (1 + tweensize) * lines_needed + tweensize);
        float horzavail = getZoom().span_x();
        const IDBAdjustment *idba = factions[fid].faction->adjustment[faction_mode];
        
        vector<pair<pair<string, bool>, vector<string> > > adjusttext;
        int total_lines = 0;
        for(int i = 0; i < ARRAY_SIZE(idba->adjustlist); i++) {
          if(idba->adjustlist[i].first == -1)
            break;
          
          vector<string> tlins;
          pair<string, bool> modifiertext = adjust_modifiertext(idba->adjustlist[i].first, idba->adjustlist[i].second);
          if(getTextWidth(StringPrintf("%s  %s", adjust_human[idba->adjustlist[i].first], modifiertext.first.c_str()), 1.0) > horzavail) {
            tlins = tokenize(adjust_human[idba->adjustlist[i].first], " ");
          } else {
            tlins.push_back(adjust_human[idba->adjustlist[i].first]);
          }
          for(int j = 1; j < tlins.size(); j++)
            tlins[j] = "  " + tlins[j];
          total_lines += tlins.size();
          
          adjusttext.push_back(make_pair(modifiertext, tlins));
        }
        
        sort(adjusttext.begin(), adjusttext.end(), AdjustSorter());
        
        float cpos = tweensize + (lines_needed - total_lines) * (1 + tweensize) / 2;
        for(int i = 0; i < adjusttext.size(); i++) {
          setColor(C::inactive_text);
          for(int j = 0; j < adjusttext[i].second.size(); j++) {
            drawText(adjusttext[i].second[j], 1, Float2(tweensize, cpos));
            cpos += 1 + tweensize;
          }
          
          if(adjusttext[i].first.second)
            setColor(C::positive);
          else
            setColor(C::negative);
          drawJustifiedText(adjusttext[i].first.first, 1, Float2(getZoom().ex - tweensize, cpos - 1 - tweensize), TEXT_MAX, TEXT_MIN);
        }
      }
    }
    
  } else if(slt.type == Slot::SHOP) {
    StackString stp("Shop");
    slt.shop.renderToScreen(&playerdata[playerid[slt.pid]]);
  } else if(slt.type == Slot::RESULTS) {
    StackString stp("Results");
    CHECK(lrCategory.size()); // make sure we *have* results
    setZoomVertical(0, 0, 600);
    setColor(1.0, 1.0, 1.0);
    
    float cury = 20;
    
    setColor(C::inactive_text);
    /*drawJustifiedText(StringPrintf("Base income: %s", lrBaseCash.textual().c_str()), 10, Float2(40 , cury), TEXT_MIN, TEXT_MIN);
    drawJustifiedText(StringPrintf("Highest player cash: %s", highestPlayerCash.textual().c_str()), 10, Float2(getZoom().ex - 40, cury), TEXT_MAX, TEXT_MIN);
    cury += 20;
    
    drawJustifiedText(StringPrintf("Starting cash: %s", newPlayerStartingCash.textual().c_str()), 10, Float2(getZoom().ex - 40, cury), TEXT_MAX, TEXT_MIN);
    cury += 20;*/
    
    cury += 40;
    
    setColor(C::inactive_text);
    drawText("Damage", 30, Float2(40, cury));
    drawMultibar(lrCategory[0], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    setColor(C::inactive_text);
    drawText("Kills", 30, Float2(40, cury));
    drawMultibar(lrCategory[1], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    setColor(C::inactive_text);
    drawText("Wins", 30, Float2(40, cury));
    drawMultibar(lrCategory[2], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    cury += 40;
    
    setColor(C::inactive_text);
    drawText("Income", 30, Float2(40, cury));
    drawMultibar(lrPlayer, Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 100;
    
    setColor(C::inactive_text);
    drawJustifiedText("Waiting for", 30, Float2(getZoom().ex / 2, cury), TEXT_CENTER, TEXT_MIN);
    cury += 40;
    
    int notdone = count(checked.begin(), checked.end(), false);
    CHECK(notdone);
    int cpos = 0;
    float increment = getZoom().ex / notdone;
    for(int i = 0; i < checked.size(); i++) {
      if(!checked[i]) {
        setColor(playerdata[i].getFaction()->color);
        drawDvec2(playerdata[i].getFaction()->icon, boxAround(Float2((cpos + 0.5) * increment, float(cury + 580) / 2), min(increment * 0.95f, float(580 - cury)) / 2), 50, 1);
        cpos++;
      }
    }
  } else if(slt.type == Slot::GAMEEND) {
    StackString stp("Gameend");
    
    vector<vector<Coord> > results(3);
    for(int i = 0; i < playerdata.size(); i++) {
      results[0].push_back(playerdata[i].total_damageDone);
      results[1].push_back(playerdata[i].total_kills);
      results[2].push_back(playerdata[i].total_wins);
    }
    
    for(int i = 0; i < results.size(); i++) {
      Coord tot = accumulate(results[i].begin(), results[i].end(), Coord(0));
      if(tot != 0)
        for(int j = 0; j < results[i].size(); j++)
          results[i][j] /= tot;
    }
    
    results.resize(results.size() + 1);
    for(int i = 0; i < playerdata.size(); i++) {
      results.back().push_back(0);
      for(int j = 0; j < results.size() - 1; j++)
        results.back()[i] += results[j][i] * value_ratios[j];
    }
    
    {
      Coord ttot = accumulate(results.back().begin(), results.back().end(), Coord(0));
      if(ttot == 0)
        ttot = 1; // you are all incompetent
      CHECK(ttot > 0);
      for(int j = 0; j < results.back().size(); j++)
        results.back()[j] /= ttot;
    }
    
    setZoomVertical(0, 0, 600);
    setColor(1.0, 1.0, 1.0);
    
    float cury = 20;
    
    setColor(C::inactive_text);
    drawJustifiedText("Final scores", 30, Float2(getZoom().ex / 2, cury), TEXT_CENTER, TEXT_MIN);
    cury += 60;
    
    setColor(C::inactive_text);
    drawText("Damage", 30, Float2(40, cury));
    drawMultibar(results[0], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    setColor(C::inactive_text);
    drawText("Kills", 30, Float2(40, cury));
    drawMultibar(results[1], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    setColor(C::inactive_text);
    drawText("Wins", 30, Float2(40, cury));
    drawMultibar(results[2], Float4(200, cury, getZoom().ex - 40, cury + 40));
    cury += 60;
    
    cury += 40;
    
    setColor(C::inactive_text);
    drawText("Totals", 30, Float2(40, cury));
    {
      vector<int> roundcount;
      for(int i = 0; i < playerdata.size(); i++)
        roundcount.push_back(playerdata[i].total_rounds);
      drawMultibar(results[3], Float4(200, cury, getZoom().ex - 40, cury + 40), &roundcount);
    }
    cury += 100;
    
    int notdone = count(checked.begin(), checked.end(), false);
    CHECK(notdone);
    int cpos = 0;
    float increment = getZoom().ex / notdone;
    for(int i = 0; i < checked.size(); i++) {
      if(!checked[i]) {
        setColor(playerdata[i].getFaction()->color);
        drawDvec2(playerdata[i].getFaction()->icon, boxAround(Float2((cpos + 0.5) * increment, float(cury + 580) / 2), min(increment * 0.95f, float(580 - cury)) / 2), 50, 1);
        cpos++;
      }
    }
  } else if(slt.type == Slot::SETTINGS) {
    CHECK(slt.pid >= 0 && slt.pid < pms.size());
    const FactionState &tfs = *pms[slt.pid].faction;
    Coord2 sizes(tfs.compass_location.span_x(), tfs.compass_location.span_y());
    Coord2 mp = tfs.compass_location.midpoint();
    setZoomAround(Coord4(mp.x - sizes.x, mp.y - sizes.y, mp.x + sizes.x, mp.y + sizes.y).toFloat());
    runSettingRender(pms[slt.pid], is.getcc(slt.pid));
  } else if(slt.type == Slot::QUITCONFIRM) {
    setZoomCenter(0, 0, 10);
    {
      setColor(pms[slt.pid].faction->faction->color * 0.5);
      const float ofs = 0.08;
      Float4 pos = getZoom();
      const float diff = pos.span_y() * ofs;
      pos.sx += diff;
      pos.sy += diff;
      pos.ex -= diff;
      pos.ey -= diff;
      drawDvec2(pms[slt.pid].faction->faction->icon, pos, 50, 0.2);
    }
    
    {
      string areyousuretext = "Are you sure you want to destroy your tank and possessions and quit the game?";
      Float4 zone = Float4(-12, -7, 12, 7);
      float height = getFormattedTextHeight(areyousuretext, 1, zone.span_x());
      
      Float4 box = extend(Float4(zone.sx, zone.sy, zone.ex, zone.sy + height), 0.4);
      drawSolid(box);
      setColor(C::inactive_text);
      drawRect(box, 0.05);
      
      setColor(C::active_text);
      drawFormattedText(areyousuretext, 1, zone);
    }
    
    {
      Float4 box = extend(Float4(-12, 0, 12, 5 * 1.5 - .5), 0.4);
      drawSolid(box);
      setColor(C::inactive_text);
      drawRect(box, 0.05);
    }
    
    for(int i = 0; i < 5; i++) {
      if(sps_quitconfirm[slt.pid] == i) {
        setColor(C::active_text);
      } else {
        setColor(C::inactive_text);
      }
      
      if(i == 3) {
        drawText("Yes, quit", 1, Float2(-12, i * 1.5));
      } else {
        drawText("No", 1, Float2(-12, i * 1.5));
      }
    }
  } else {
    CHECK(0);
  }
}

bool PersistentData::isUnfinished(int id) const {
  if(sps_playermode[id] == SPS_DONE || sps_playermode[id] == SPS_END)
    return false;
  
  if(!pms[id].faction && (sps_playermode[id] == SPS_IDLE || sps_playermode[id] == SPS_CHOOSING))
    return false;
  
  return true;
}

vector<const IDBFaction *> PersistentData::getUnfinishedFactions() const {
  StackString sst("guf");
  
  vector<const IDBFaction *> nrfactions;
  for(int i = 0; i < pms.size(); i++) {
    if(isUnfinished(i)) {
      if(pms[i].faction)
        nrfactions.push_back(pms[i].faction->faction);
      else
        nrfactions.push_back(NULL);
    }
  }
  return nrfactions;
}

vector<const IDBFaction *> PersistentData::getUnfinishedHumanFactions() const {
  StackString sst("guf");
  
  vector<const IDBFaction *> nrfactions;
  for(int i = 0; i < pms.size(); i++) {
    if(humans[i] && isUnfinished(i)) {
      if(pms[i].faction)
        nrfactions.push_back(pms[i].faction->faction);
      else
        nrfactions.push_back(NULL);
    }
  }
  return nrfactions;
}

bool PersistentData::onlyAiUnfinished() const {
  for(int i = 0; i < pms.size(); i++)
    if(humans[i] && isUnfinished(i))
      return false;
  
  return true;
}

vector<Ai *> PersistentData::distillAi(const vector<Ai *> &ai) const {
  CHECK(ai.size() == pms.size());
  vector<Ai *> rv(playerdata.size());
  for(int i = 0; i < pms.size(); i++) {
    if(playerid[i] != -1)
      rv[playerid[i]] = ai[i];
  }
  
  for(int i = 0; i < ai.size(); i++)
    if(ai[i])
      CHECK(count(rv.begin(), rv.end(), ai[i]) <= 1);
  
  return rv;
}

void PersistentData::setFactionMode(int in_faction_mode) {
  CHECK(in_faction_mode >= 0 && in_faction_mode < 4);
  faction_mode = in_faction_mode;
  for(int i = 0; i < playerdata.size(); i++)
    playerdata[i].setFactionMode(faction_mode);
}

void PersistentData::ai(const vector<Ai *> &ais, const vector<bool> &isHuman) const {
  StackString sst("persistent AI");
  set<Ai *> untouched;
  for(int i = 0; i < ais.size(); i++)
    if(ais[i])
      untouched.insert(ais[i]);
  
  CHECK(humans == isHuman);
  
  if(mode == TM_RESULTS) {
    for(int i = 0; i < playerid.size(); i++) {
      //dprintf("%d, %d, %d\n", i, playerid.size(), checked.size());
      //dprintf("%d\n", playerid[i]);
      //dprintf("%d\n", checked[i]);
      //dprintf("WFR %d %d %08x (%d)\n", i, playerid[i], ais[i], checked[i]);
      if(playerid[i] != -1 && ais[i]) {
        CHECK(untouched.count(ais[i]));
        untouched.erase(ais[i]);
        ais[i]->updateWaitingForReport(checked[playerid[i]]);
      }
    }
  } else if(mode == TM_GAMEEND) {
    for(int i = 0; i < playerid.size(); i++) {
      //dprintf("GE %d %d %08x (%d)\n", i, playerid[i], ais[playerid[i]], checked[i]);
      if(playerid[i] != -1 && ais[i]) {
        CHECK(untouched.count(ais[i]));
        untouched.erase(ais[i]);
        ais[i]->updateGameEnd(checked[playerid[i]]);
      }
    }
  } else if(mode == TM_SHOP || mode == TM_PLAYERCHOOSE) {
    // First, we want to see if the players exist and are done.
    // If players exist, and there is one in-game, and they're all done, then we can do AI.
    // If no players exist, then we can do AI.
    // Conveniently, this is almost the same logic as isWaitingOnAi(). The only difference is that, if there's no humans, we really want to continue.
    
    if(count(isHuman.begin(), isHuman.end(), true) && !isWaitingOnAi() && count(isHuman.begin(), isHuman.end(), false)) {
      // There are humans, and we're not waiting on the AI, and there are actual AIs. Have the AIs idle.
      for(int i = 0; i < ais.size(); i++) {
        if(ais[i]) {
          CHECK(!isHuman[i]);
          ais[i]->updateIdle();
        }
      }
    } else {
      vector<bool> dun(ais.size(), false);
      
      int shops = 0;
      int chooses = 0;
      int tweens = 0;
      
      for(int i = 0; i < slot_count; i++) {
        if(slot[i].type != Slot::EMPTY) {
          CHECK(slot[i].pid != -1); // blah
          CHECK(!dun[slot[i].pid]);
          dun[slot[i].pid] = true;
          if(!ais[slot[i].pid])
            continue;
          
          CHECK(untouched.count(ais[slot[i].pid]));
          untouched.erase(ais[slot[i].pid]);
          
          if(slot[i].type == Slot::SHOP) {
            slot[i].shop.ai(ais[slot[i].pid], &playerdata[playerid[slot[i].pid]]);
            shops++;
          } else if(slot[i].type == Slot::CHOOSE || slot[i].type == Slot::SETTINGS) {
            ais[slot[i].pid]->updateCharacterChoice(factions, pms[slot[i].pid]);
            chooses++;
          } else {
            dprintf("%d\n", slot[i].type);
            CHECK(0);
          }
        }
      }
      for(int i = 0; i < dun.size(); i++) {
        if(!dun[i]) {
          dun[i] = true;
          
          if(!ais[i])
            continue;
          Coord2 endcoords;
          Coord2 joincoords;
          Coord2 quickshopcoords;
          bool shopped;
          if(mode == TM_SHOP) {
            joincoords = targetCoords(TTL_LEAVEJOIN);
            quickshopcoords = targetCoords(TTL_SHOP);
            shopped = sps_shopped[i];
          } else {
            joincoords = targetCoords(TTL_JOIN);
            quickshopcoords = Coord2(0, 0);
            shopped = true;
          }
          
          if(shopcycles) {
            endcoords = targetCoords(TTL_END);
          } else {
            endcoords = Coord2(0, 0);
          }
          
          CHECK(untouched.count(ais[i]));
          untouched.erase(ais[i]);
          
          ais[i]->updateTween(!!pms[i].faction, sps_playermode[i] == SPS_PENDING, sps_playerpos[i], shopped, joincoords, quickshopcoords, targetCoords(TTL_DONE), endcoords);
          tweens++;
        }
      }
      int factioned = 0;
      for(int i = 0; i < pms.size(); i++)
        if(pms[i].faction)
          factioned++;
      
      CHECK(dun == vector<bool>(ais.size(), true));
    }
  } else {
    CHECK(0);
  }
  
  for(set<Ai *>::iterator itr = untouched.begin(); itr != untouched.end(); itr++)
    (*itr)->updateIdle();
}

bool PersistentData::isWaitingOnAi() const {
  if((mode == TM_PLAYERCHOOSE || mode == TM_SHOP) && getHumanCount() && getAiCount()) {
    // If there are any humans, we've got a few questions. First, if there are no humans ready, we're not waiting on the AI guaranteed.
    {
      bool hasReadyPlayer = false;
      for(int i = 0; i < humans.size(); i++) {
        if(humans[i] && (sps_playermode[i] == SPS_DONE || sps_playermode[i] == SPS_END))
          hasReadyPlayer = true;
      }
      if(!hasReadyPlayer)
        return false;
    }
    // If there are finished humans, we check to see if there's only AIs left.
    return onlyAiUnfinished();
  } else {
    // If there are no humans, or no ais, we might as well return false just so we avoid skipping stuff.
    return false;
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

vector<bool> PersistentData::genHumans(const vector<bool> &human) const {
  vector<bool> kst(playerdata.size());
  int ct = 0;
  set<int> kstd;
  for(int i = 0; i < pms.size(); i++) {
    if(pms[i].faction) {
      kst[playerid[i]] = human[i];
      ct++;
      kstd.insert(playerid[i]);
    }
  }
  CHECK(kstd.size() == ct);
  CHECK(ct == kst.size());
  return kst;
}

void PersistentData::divvyCash(int rounds) {
  if(rounds == -1)
    rounds = roundsbetweenshop;
  
  CHECK(playerdata.size());
  shopcycles++;
  
  checked.clear();
  checked.resize(playerdata.size());
  
  vector<vector<Coord> > values(4);
  for(int i = 0; i < playerdata.size(); i++) {
    values[0].push_back(playerdata[i].consumeDamage());
    values[1].push_back(playerdata[i].consumeKills());
    values[2].push_back(playerdata[i].consumeWins());
    values[3].push_back(1);
  }
  vector<Coord> totals(values.size());
  for(int j = 0; j < totals.size(); j++) {
    totals[j] = accumulate(values[j].begin(), values[j].end(), Coord(0));
  }
  
  Coord chunkTotal = 0;
  for(int i = 0; i < totals.size(); i++) {
    if(totals[i] > 1e-6)
      chunkTotal += value_ratios[i];
  }
  
  // We give the users a good chunk of money at the beginning to get started, but then we tone it down a bit per round so they don't get an immediate 6x increase. (or whateverx increase.) In a lot of ways, "starting cash" is a crummy number - it should be "starting cash per round", with starting cash calculated from that. But it's easier to understand this way.
  lrBaseCash = Money((long long)((100. / 1000 * baseStartingCash.value()) * powl(multiplePerRound.toFloat(), roundsbetweenshop * shopcycles) * playerdata.size() * rounds));
  double total = lrBaseCash.value();
  dprintf("Total cash is %f", total);
  
  if(total > 1e3000) {
    total = 1e3000;
    dprintf("Adjusted to 1e3000\n");
  }
  
  for(int i = 0; i < playerdata.size(); i++) {
    for(int j = 0; j < totals.size(); j++) {
      if(totals[j] > 1e-6)
        values[j][i] /= totals[j];
    }
  }
  // values now stores percentages for each category
  
  vector<Coord> playercash(playerdata.size());
  vector<Coord> playerresult(playerdata.size());
  for(int i = 0; i < playercash.size(); i++) {
    for(int j = 0; j < totals.size(); j++) {
      playercash[i] += values[j][i] * value_ratios[j];
      if(j != totals.size() - 1)
        playerresult[i] += values[j][i] * value_ratios[j];
    }
    playercash[i] /= chunkTotal;
    playerresult[i] /= chunkTotal - value_ratios[totals.size() - 1];
  }
  // playercash now stores percentages for players
  
  vector<Money> playercashresult(playerdata.size());
  for(int i = 0; i < playercash.size(); i++) {
    playercashresult[i] = Money((long long)(playercash[i].toFloat() * total));
  }
  // playercashresult now stores cashola for players
  
  lrCategory = values;
  lrPlayer = playerresult;
  lrCash = playercashresult;
  
  mode = TM_RESULTS;
  slot_count = 1;
  slot[0].type = Slot::RESULTS;
  slot[0].pid = -1;
  
  newPlayerStartingCash = playerdata[0].totalValue() + lrCash[0];
  for(int i = 0; i < playerdata.size(); i++) {
    newPlayerStartingCash = min(newPlayerStartingCash, playerdata[i].totalValue() + lrCash[i]);
    highestPlayerCash = max(highestPlayerCash, playerdata[i].totalValue() + lrCash[i]);
  }
  newPlayerStartingCash = newPlayerStartingCash * 0.8;
  newPlayerStartingCash = max(newPlayerStartingCash, baseStartingCash);
  
  newPlayerDamageDone = 0;
  newPlayerKills = 0;
  newPlayerWins = 0;
  
  for(int i = 0; i < playerdata.size(); i++) {
    playerdata[i].total_damageDone += values[0][i];
    playerdata[i].total_kills += values[1][i];
    playerdata[i].total_wins += values[2][i];
    playerdata[i].total_rounds += rounds;
    
    newPlayerDamageDone += playerdata[i].total_damageDone;
    newPlayerKills = playerdata[i].total_kills;
    newPlayerWins = playerdata[i].total_wins;
  }
  
  newPlayerDamageDone /= playerdata.size();
  newPlayerKills /= playerdata.size();
  newPlayerWins /= playerdata.size();
}

void PersistentData::endgame(int rounds, bool playing) {
  if(playerdata.size() || playing)
    divvyCash(rounds);
  
  rounds_until_end = 0;
  
  bool has_rounds = false;
  for(int i = 0; i < playerdata.size(); i++)
    if(playerdata[i].total_rounds)
      has_rounds = true;
  if(!has_rounds || !playing)
    enterGameEnd();
}

void PersistentData::startAtNormalShop() {
  mode = TM_SHOP;
  reset();
  slot[0].type = Slot::EMPTY;
}

void PersistentData::resetRanges() {
  vector<int> avails;
  if(mode == TM_PLAYERCHOOSE) {
    avails.push_back(TTL_LEAVE);
    avails.push_back(TTL_JOIN);
    avails.push_back(TTL_SETTINGS);
    avails.push_back(TTL_DONE);
  } else if(mode == TM_SHOP) {
    if(shopcycles)
      avails.push_back(TTL_END);
    avails.push_back(TTL_LEAVEJOIN);
    avails.push_back(TTL_SETTINGS);
    avails.push_back(TTL_SHOP);
    avails.push_back(TTL_DONE);
  } else {
    CHECK(0);
  }
  
  ranges.clear();
  range_targets.clear();
  range_targets.resize(TTL_LAST, make_pair(Coord(0), Coord(0)));
  for(int i = 0; i < avails.size(); i++) {
    vector<string> lines = tokenize(tween_textlabels[avails[i]], " ");
    const Coord pivot = 100. / (avails.size() * 2) * (i * 2 + 1);
    
    float mwid = 0;
    for(int j = 0; j < lines.size(); j++)
      mwid = max(mwid, getTextWidth(lines[j], ticker_text_size));
    mwid += 2;
    
    pair<Coord, Coord> item = make_pair(pivot - mwid / 2, pivot + mwid / 2);
    ranges.push_back(make_pair(avails[i], item)); 
    CHECK(range_targets[avails[i]].second == 0);
    range_targets[avails[i]] = item;
  }
}

Coord2 PersistentData::targetCoords(int target) const {
  CHECK(range_targets[target].second != 0);
  return Coord2((range_targets[target].first + range_targets[target].second) / 2, 95);
}
  
void PersistentData::drawMultibar(const vector<Coord> &sizes, const Float4 &dimensions, const vector<int> *roundcount) const {
  CHECK(sizes.size() == playerdata.size());
  CHECK(!roundcount || roundcount->size() == playerdata.size());
  float total = accumulate(sizes.begin(), sizes.end(), Coord(0)).toFloat();
  if(total < 1e-6)
    return;
  vector<pair<float, int> > order;
  for(int i = 0; i < sizes.size(); i++)
    order.push_back(make_pair(sizes[i].toFloat(), i));
  sort(order.begin(), order.end());
  reverse(order.begin(), order.end());
  float width = dimensions.ex - dimensions.sx;
  float per = width / total;
  float cpos = dimensions.sx;
  float barbottom = (dimensions.ey - dimensions.sy) * 3 / 4 + dimensions.sy;
  float iconsize = (dimensions.ey - dimensions.sy) / 4 * 0.9;
  float iconoffs = 0;
  if(roundcount)
    iconoffs = -iconsize;
  for(int i = 0; i < order.size(); i++) {
    if(order[i].first == 0)
      continue;
    setColor(playerdata[order[i].second].getFaction()->color);
    float epos = cpos + order[i].first * per;
    drawShadedRect(Float4(cpos, dimensions.sy, epos, barbottom), 1, 6);
    drawDvec2(playerdata[order[i].second].getFaction()->icon, boxAround(Float2((cpos + epos) / 2 + iconoffs, (dimensions.ey + (dimensions.ey - iconsize)) / 2), iconsize / 2), 20, 0.1);
    if(roundcount)
      drawJustifiedText(StringPrintf("%d", (*roundcount)[order[i].second]), iconsize, Float2((cpos + epos) / 2 - iconoffs, (dimensions.ey + (dimensions.ey - iconsize)) / 2), TEXT_MIN, TEXT_CENTER);
    cpos = epos;
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

void PersistentData::attemptQueueSound(int player, const Sound *sound) {
  if(sps_soundtimeout[player] <= 0) {
    queueSound(sound);
    sps_soundtimeout[player] = FPS;
  }
}

// DESTROY

void PersistentData::destroyPlayer(int pid) {
  // DESTROY
  dprintf("DESTROY %d\n", playerdata.size());
  int spid = playerid[pid];
  pms[pid].faction->taken = false;
  // DESTROY
  playerid[pid] = -1;
  sps_shopped[pid] = false;
  sps_playermode[pid] = SPS_IDLE;
  pms[pid] = PlayerMenuState();
  playerdata.erase(playerdata.begin() + spid);
  // DESTROY
  for(int i = 0; i < playerid.size(); i++)
    if(playerid[i] > spid)
      playerid[i]--;
  dprintf("DESTROY %d\n", playerdata.size());
  // DESTROY
}

// DESTROY

void PersistentData::enterGameEnd() {
  slot_count = 1;
  slot[0].type = Slot::GAMEEND;
  slot[0].pid = -1;
  mode = TM_GAMEEND;
  checked.clear();
  checked.resize(playerdata.size());
}

int PersistentData::getExpectedPlayercount() const {
  CHECK(humans.size());
  int cai = 0;
  for(int i = 0; i < pms.size(); i++)
    if(playerid[i] != -1 && !humans[i])
      cai++;
  
  return min(playerdata.size() - cai + getAiCount(), factions.size());
}

int PersistentData::getHumanCount() const {
  CHECK(humans.size());
  int hc = 0;
  for(int i = 0; i < pms.size(); i++)
    if(playerid[i] != -1 && humans[i])
      hc++;
  return hc;
};

int PersistentData::getAiCount() const {
  CHECK(humans.size());
  return count(humans.begin(), humans.end(), false);
};

DEFINE_int(debugControllers, 0, "Number of controllers to set to debug defaults");
REGISTER_int(debugControllers);

PersistentData::PersistentData(const vector<bool> &human, Money startingcash, Coord multiple, int in_roundsbetweenshop, int in_rounds_until_end, const InputSnag &isnag) {
  CHECK(multiple > 1);
  roundsbetweenshop = in_roundsbetweenshop;
  rounds_until_end = in_rounds_until_end;
  faction_mode = 0;
  humans = human;
  
  const int playercount = human.size();
  
  pms.clear();
  pms.resize(playercount);
  playerid.resize(playercount, -1);
  sps_quitconfirm.resize(playercount, 0);
  sps_soundtimeout.resize(playercount, 0);
  
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
        bool neg = factcents.second[i].y > 0;
        centangs.push_back(make_pair(getAngle(Float2(factcents.second[i].x, sqrt(abs(factcents.second[i].y)) * (neg * 2 - 1))), factcents.second[i]));
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
    
    for(int i = 0; i < facthues.size(); i++)
      dprintf("%f %f %f: %s\n", facthues[i].first, factions[facthues[i].second].faction->color.getSaturation(), factions[facthues[i].second].faction->color.getValue(), factions[facthues[i].second].faction->name.c_str());
    //CHECK(0);
    
    CHECK(centgrays.size() == factgrays.size());
    CHECK(centangs.size() == facthues.size());  // this is kind of flimsy
    
    for(int i = 0; i < facthues.size(); i++)
      factions[facthues[i].second].compass_location = Coord4(factcents.first + centangs[i].second);
    for(int i = 0; i < factgrays.size(); i++)
      factions[factgrays[i]].compass_location = Coord4(factcents.first + centgrays[i]);
  }
  
  mode = TM_PLAYERCHOOSE;
  
  shopcycles = 0;
  
  resetRanges();
  
  CHECK(FLAGS_debugControllers >= 0 && FLAGS_debugControllers <= 3);
  CHECK(factions.size() >= FLAGS_debugControllers);
  
  CHECK(ARRAY_SIZE(slot) == 4);
  for(int i = 0; i < 4; i++) {
    slot[i].type = Slot::EMPTY;
    slot[i].pid = -1;
  }
  
  slot_count = 1;
  
  btt_notify = NULL;
  btt_frames_left = 0;
  
  baseStartingCash = startingcash;
  multiplePerRound = multiple;
  
  newPlayerStartingCash = baseStartingCash;
  highestPlayerCash = baseStartingCash;
  
  lrBaseCash = Money(0);
  
  reset();
  
  int cdbc = isnag.primary_id();
  if(FLAGS_debugControllers >= 1) {
    CHECK(pms.size() >= 1); // better be
    
    const int fact = 5;
    pms[cdbc].faction = &factions[fact];
    factions[fact].taken = true;
    playerid[cdbc] = playerdata.size();
    
    playerdata.push_back(Player(pms[cdbc].faction->faction, faction_mode, newPlayerStartingCash));
    
    Controller tc;
    tc.axes.resize(isnag.getaxiscount(cdbc));
    tc.lastaxes.resize(isnag.getaxiscount(cdbc));
    tc.keys.resize(isnag.getbuttoncount(cdbc));
    pms[cdbc].settingmode = SETTING_BUTTONS;
    pms[cdbc].choicemode = CHOICE_FIRSTPASS;
    runSettingTick(tc, &pms[cdbc], factions, isnag.getcc(cdbc));
    pms[cdbc].settingmode = SETTING_READY;
    pms[cdbc].choicemode = CHOICE_IDLE;
    
    cdbc++;
  }
  if(FLAGS_debugControllers >= 2) {
    CHECK(pms.size() >= 2); // better be
    
    const int fact = 6;
    pms[cdbc].faction = &factions[fact];
    factions[fact].taken = true;
    playerid[cdbc] = playerdata.size();
    
    playerdata.push_back(Player(pms[cdbc].faction->faction, faction_mode, newPlayerStartingCash));
    
    Controller tc;
    tc.axes.resize(isnag.getaxiscount(cdbc));
    tc.lastaxes.resize(isnag.getaxiscount(cdbc));
    tc.keys.resize(isnag.getbuttoncount(cdbc));
    pms[cdbc].settingmode = SETTING_BUTTONS;
    pms[cdbc].choicemode = CHOICE_FIRSTPASS;
    runSettingTick(tc, &pms[cdbc], factions, isnag.getcc(cdbc));
    pms[cdbc].settingmode = SETTING_READY;
    pms[cdbc].choicemode = CHOICE_IDLE;
    
    cdbc++;
  }
  if(FLAGS_debugControllers >= 3) {
    CHECK(pms.size() >= 3);
    const int fact = 3;
    pms[cdbc].faction = &factions[fact];
    factions[fact].taken = true;
    pms[cdbc].settingmode = SETTING_READY;
    pms[cdbc].choicemode = CHOICE_IDLE;
    pms[cdbc].buttons[BUTTON_FIRE1] = 4;
    pms[cdbc].buttons[BUTTON_FIRE2] = 5;
    pms[cdbc].buttons[BUTTON_FIRE3] = 6;
    pms[cdbc].buttons[BUTTON_FIRE4] = 7;
    pms[cdbc].buttons[BUTTON_PRECISION] = 8;
    pms[cdbc].buttons[BUTTON_ACCEPT] = 2;
    pms[cdbc].buttons[BUTTON_CANCEL] = 1;
    CHECK(pms[cdbc].buttons.size() == 7);
    pms[cdbc].axes[0] = 1;
    pms[cdbc].axes[1] = 3;
    pms[cdbc].axes_invert[0] = false;
    pms[cdbc].axes_invert[1] = false;
    pms[cdbc].setting_axistype = KSAX_STEERING;
    playerid[cdbc] = playerdata.size();
    playerdata.push_back(Player(pms[cdbc].faction->faction, faction_mode, newPlayerStartingCash));
    cdbc++;
  }
}

void buyAndSlot(Player *player, const IDBWeapon *weapon, int amount, int slot) {
  player->forceAcquireWeapon(weapon, amount);
  player->promoteWeapon(weapon, slot);
}

void PersistentData::instant_action_init(const ControlConsts &cc, int primary_id) {
  CHECK(pms.size() >= 1); // better be
  CHECK(playerdata.size() == 0);
  
  int cdbc = primary_id;
  pms[cdbc].faction = &factions[1];
  pms[cdbc].faction->taken = true;
  pms[cdbc].settingmode = SETTING_BUTTONS;
  
  CHECK(cc.ck.canned);
  Controller cntr;
  cntr.keys.resize(17);
  cntr.axes.resize(4);
  cntr.lastaxes = cntr.axes;
  runSettingTick(cntr, &pms[cdbc], factions, cc);
  
  pms[cdbc].settingmode = SETTING_READY;
  pms[cdbc].choicemode = CHOICE_IDLE;
  sps_playermode[cdbc] = SPS_DONE;
  
  playerid[cdbc] = playerdata.size();
  playerdata.push_back(Player(pms[cdbc].faction->faction, faction_mode, newPlayerStartingCash));
  
  // Purchase shit
  buyAndSlot(&playerdata.back(), getWeapon("ROOT.Ammo.Autocannon.Autocannon II"), 600, 0);
  buyAndSlot(&playerdata.back(), getWeapon("ROOT.Ammo.Laser.Laser II"), 600, 1);
  buyAndSlot(&playerdata.back(), getWeapon("ROOT.Ammo.Missile.Missile II"), 600, 2);
  buyAndSlot(&playerdata.back(), getWeapon("ROOT.Ammo.EMP.EMP II"), 75, 3);
  playerdata.back().forceNonCorrupted();
}
