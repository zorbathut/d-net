
#include "metagame_tween.h"

#include <numeric>

#include "args.h"
#include "gfx.h"
#include "ai.h"

using namespace std;

bool PersistentData::isPlayerChoose() const {
  return mode == TM_PLAYERCHOOSE;
}

vector<Player> &PersistentData::players() {
  return playerdata;
}

bool PersistentData::tick(const vector< Controller > &keys) {
  CHECK(keys.size() == pms.size());
  
  if(mode == TM_PLAYERCHOOSE) {
    StackString stp("Playerchoose");
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
        mode = TM_SHOP;
        
        playerdata.clear();
        playerdata.resize(readyusers);
        int pid = 0;
        for(int i = 0; i < pms.size(); i++) {
          if(pms[i].faction) {
            playerdata[pid] = Player(pms[i].faction->faction, 0);
            pid++;
          }
        }
        CHECK(pid == playerdata.size());
        
        currentShop = 0;
        shop.init(&playerdata[0], true);  // this is so hideous
        return true;
      }
    }
  } else if(mode == TM_SHOP) {
    vector<Keystates> ki = genKeystates(keys);
    if(currentShop == -1) {
      StackString stp("Results");
      // this is a bit hacky - SHOP mode when currentShop is -1 is the "show results" screen
      for(int i = 0; i < ki.size(); i++) {
        CHECK(SIMUL_WEAPONS == 2);
        if(ki[i].accept.push || ki[i].fire[0].push || ki[i].fire[1].push)
          checked[i] = true;
      }
      if(count(checked.begin(), checked.end(), false) == 0) {
        for(int i = 0; i < playerdata.size(); i++)
          playerdata[i].addCash(lrCash[i]);
        currentShop = 0;
        shop.init(&playerdata[0], true);
      }
    } else if(shop.runTick(ki[currentShop])) {
      StackString stp("Shop");
      // and here's our actual shop - the tickrunning happens in the conditional, this is just what happens if it's time to change shops
      currentShop++;
      if(currentShop != playerdata.size()) {
        shop.init(&playerdata[currentShop], true);
      } else {
        currentShop = -1; // eugh
        return true;
      }
    }
  } else {
    CHECK(0);
  }
  return false;
}

void PersistentData::render() const {
  if(mode == TM_PLAYERCHOOSE) {
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
  } else if(mode == TM_SHOP) {
    if(currentShop == -1) {
      StackString stp("Results");
      setZoom(Float4(0, 0, 800, 600));
      setColor(1.0, 1.0, 1.0);
      drawText("Damage", 30, 20, 20);
      drawText("Kills", 30, 20, 80);
      drawText("Wins", 30, 20, 140);
      drawText("Base", 30, 20, 200);
      drawText("Totals", 30, 20, 320);
      drawMultibar(lrCategory[0], Float4(200, 20, 700, 60));
      drawMultibar(lrCategory[1], Float4(200, 80, 700, 120));
      drawMultibar(lrCategory[2], Float4(200, 140, 700, 180));
      drawMultibar(lrCategory[3], Float4(200, 200, 700, 240));
      drawMultibar(lrPlayer, Float4(200, 320, 700, 360));
      setColor(1.0, 1.0, 1.0);
      drawJustifiedText("Waiting for", 30, 400, 400, TEXT_CENTER, TEXT_MIN);
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
      StackString stp("Shop");
      
      const float divider_pos = 90;
      
      setZoom(Float4(0, 0, 133.333, 100));
      setColor(1.0, 1.0, 1.0);
      drawLine(Float4(0, divider_pos, 140, divider_pos), 0.1);
      
      GfxWindow gfxw(Float4(0, 0, 133.333, divider_pos), 1.0);
      
      setZoom(Float4(0, 0, getAspect(), 1.0));
      
      drawLine(Float4(0, 0.5, getAspect(), 0.5), 0.001);
      drawLine(Float4(getAspect() / 2, 0, getAspect() / 2, 1), 0.001);
      
      {
        GfxWindow gfxw2(Float4(0, 0, getAspect() / 2, 0.5), 1.0);
        shop.renderToScreen();
      }
      {
        GfxWindow gfxw2(Float4(getAspect() / 2, 0, getAspect(), 0.5), 1.0);
        shop.renderToScreen();
      }
      {
        GfxWindow gfxw2(Float4(0, 0.5, getAspect() / 2, 1), 1.0);
        shop.renderToScreen();
      }
      {
        GfxWindow gfxw2(Float4(getAspect() / 2, 0.5, getAspect(), 1), 1.0);
        shop.renderToScreen();
      }
    }
  } else {
    CHECK(0);
  }
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
  } else if(mode == TM_SHOP) {
    if(currentShop == -1) {
      for(int i = 0; i < ais.size(); i++)
        if(ais[i])
          ais[i]->updateWaitingForReport();
    } else {
      CHECK(currentShop >= 0 && currentShop < playerdata.size());
      shop.ai(distillAi(ais)[currentShop]);
      for(int i = 0; i < ais.size(); i++)
        if(ais[i] && i != currentShop)
          ais[i]->updateIdle();
    }
  } else {
    CHECK(0);
  }
}

vector<Keystates> PersistentData::genKeystates(const vector<Controller> &keys) const {
  vector<Keystates> kst;
  for(int i = 0; i < pms.size(); i++)
    if(pms[i].faction)
      kst.push_back(pms[i].genKeystate(keys[i]));
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
}
/*


vector<GameAi *> distillGameAi(const vector<Ai *> &in_ai, const vector<PlayerMenuState> &players) {
  vector<Ai *> ai = distillAi(in_ai, players);
  vector<GameAi *> rv;
  for(int i = 0; i < ai.size(); i++) {
    if(ai[i]) {
      rv.push_back(&ai[i]->getGameAi());
    } else {
      rv.push_back(NULL);
    }
  }
  return rv;
}
*/

