
#include "metagame_config.h"

#include "game.h"
#include "gfx.h"
#include "player.h"
#include "debug.h"
#include "game_ai.h"

using namespace std;

class RenderInfo {
public:
  // Basic math!
  static const float textline_size;  // How many times larger a line of text is than its divider
  static const float border_size;
  static const float divider_size;	// Division between tabs and content
  static const float units;

  static const int textline_count = 8; // How many lines we're going to have

  // runtime constants
  Float4 drawzone;
  float unitsize;
  float border;
  float xstart;
  float xend;
  vector<float> ystarts;

  float textsize;
    
  RenderInfo(const Float4 &in_dz) {
    drawzone = in_dz;    

    // runtime constants
    unitsize = drawzone.y_span() / units;
    border = unitsize * border_size;
    xstart = drawzone.sx + unitsize * border_size;
    xend = drawzone.ex - unitsize * border_size;
    ystarts.resize(textline_count);
    ystarts[0] = drawzone.sy + unitsize * border_size;
    textsize = unitsize * textline_size;
    for(int i = 1; i < textline_count; i++)
      ystarts[i] = drawzone.sy + unitsize * (border_size + divider_size + i - 1) + textsize * i;
  }
};

// compiletime constants
const float RenderInfo::textline_size = 3;  // How many times larger a line of text is than its divider
const float RenderInfo::border_size = 1.5;
const float RenderInfo::divider_size = 3;	// Division between tabs and content
const float RenderInfo::units = textline_size * textline_count + textline_count - 1 + border_size * 2 + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders, plus the increased divider from categories to data

// brokenated :D
PlayerMenuState::PlayerMenuState() {
  settingmode = -12345;
  choicemode = -12345;

  setting_button_current = -1123;
  setting_button_reading = false;
  
  setting_axis_current = -1123;
  setting_axis_reading = false;
  
  setting_axistype = 1237539;
  setting_axistype_curchoice = 458237;
  
  faction = NULL;
  compasspos = Float2(0,0);
}

PlayerMenuState::PlayerMenuState(Float2 cent) {
  settingmode = SETTING_BUTTONS;
  choicemode = CHOICE_FIRSTPASS;
  
  setting_button_current = -1;
  setting_button_reading = true;
  
  setting_axis_current = -1;
  setting_axis_reading = true;
  
  setting_axistype = -1;
  setting_axistype_curchoice = 0;
  setting_axistype_demo_curframe = -1;
  
  buttons.resize(BUTTON_LAST, -1);
  axes.resize(2, -1);
  axes_invert.resize(axes.size(), false);
  
  faction = NULL;
  compasspos = cent;
}

PlayerMenuState::~PlayerMenuState() { }

void PlayerMenuState::createNewAxistypeDemo() {
  setting_axistype_demo_player.reset();
  setting_axistype_demo_game.reset();
  
  setting_axistype_demo_player.reset(new Player(faction->faction, 0));
  
  const RenderInfo rin(faction->compass_location);
  
  Float4 boundy = Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]);
  boundy -= boundy.midpoint();
  
  float mn = min(boundy.x_span(), boundy.y_span());
  boundy *= (100 / mn);
  
  setting_axistype_demo_game.reset(new Game());
  setting_axistype_demo_game->initCenteredDemo(setting_axistype_demo_player.get(), 50);
  
  setting_axistype_demo_ai.reset(new GameAiAxisRotater(GameAiAxisRotater::Config(false, false), KSAX_STEERING));
}

bool PlayerMenuState::readyToPlay() const {
  return faction && settingmode == SETTING_READY && fireHeld == 60;
}

Keystates genKeystate(const Controller &keys, const PlayerMenuState &pms) {
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  CHECK(SIMUL_WEAPONS == 2);
  kst.accept = keys.keys[pms.buttons[BUTTON_ACCEPT]];
  kst.cancel = keys.keys[pms.buttons[BUTTON_CANCEL]];
  kst.fire[0] = keys.keys[pms.buttons[BUTTON_FIRE1]];
  kst.fire[1] = keys.keys[pms.buttons[BUTTON_FIRE2]];
  kst.change[0] = keys.keys[pms.buttons[BUTTON_SWITCH1]];
  kst.change[1] = keys.keys[pms.buttons[BUTTON_SWITCH2]];
  kst.axmode = pms.setting_axistype;
  for(int j = 0; j < 2; j++) {
    kst.ax[j] = keys.axes[pms.axes[j]];
    if(pms.axes_invert[j])
      kst.ax[j] *= -1;
  }
  kst.udlrax = keys.menu;
  CHECK(keys.menu.x >= -1 && keys.menu.x <= 1);
  CHECK(keys.menu.y >= -1 && keys.menu.y <= 1);
  return kst;
}

vector<Keystates> genKeystates(const vector<Controller> &keys, const vector<PlayerMenuState> &modes) {
  vector<Keystates> kst;
  for(int i = 0; i < modes.size(); i++)
    if(modes[i].faction)
      kst.push_back(genKeystate(keys[i], modes[i]));
  return kst;
}

struct StandardButtonTickData {
  vector<int> *outkeys;
  vector<char> *outinvert;
  
  int *current_button;
  bool *current_mode;
  
  const int *groups;
  
  Controller keys;
  const vector<float> *triggers;
  
  PlayerMenuState *pms;
};

struct StandardButtonRenderData {
  const RenderInfo *rin;
  
  const vector<int> *buttons;
  const vector<char> *inverts;
  
  const vector<vector<string> > *names;
  const int *groups;
  
  int sel_button;
  bool sel_button_reading;
  
  float fadeFactor;
  char prefixchar;
};

void standardButtonTick(StandardButtonTickData *sbtd) {
  StackString sstr("standardButtonTick");
  if(*sbtd->current_button == -1) {
    *sbtd->current_button = 0;
    *sbtd->current_mode = (sbtd->pms->choicemode == CHOICE_FIRSTPASS);
  }
  CHECK(*sbtd->current_button >= 0 && *sbtd->current_button < sbtd->outkeys->size());
  if(*sbtd->current_mode) {
    CHECK(sbtd->pms->choicemode != CHOICE_IDLE);
    for(int i = 0; i < sbtd->triggers->size(); i++) {
      if(abs((*sbtd->triggers)[i]) > 0.9) {
        int j;
        bool noopify = false;
        
        // Step 1: If this button has been chosen somewhere else, and
        // we're not choosing this button for the first time, swap 'em (TODO: go back?)
        for(j = 0; j < sbtd->outkeys->size(); j++) {
          if(sbtd->groups[j] != sbtd->groups[*sbtd->current_button])
            continue;
          if((*sbtd->outkeys)[j] == i) {
            if((*sbtd->outkeys)[*sbtd->current_button] != -1) {
              swap((*sbtd->outkeys)[j], (*sbtd->outkeys)[*sbtd->current_button]);
              if(sbtd->outinvert) {
                swap((*sbtd->outinvert)[j], (*sbtd->outinvert)[*sbtd->current_button]);
                (*sbtd->outinvert)[*sbtd->current_button] = ((*sbtd->triggers)[i] < 0);
              }
            } else {
              // If we *are* choosing this button for the first time, cancel everything.
              noopify = true;
            }
            break;
          }
        }
        if(noopify)
          continue;
        
        // If that wasn't true, then our button is being chosen for the first time.
        if(j == sbtd->outkeys->size()) {
          (*sbtd->outkeys)[*sbtd->current_button] = i;
          if(sbtd->outinvert)
            (*sbtd->outinvert)[*sbtd->current_button] = ((*sbtd->triggers)[i] < 0);
        }
        
        if(sbtd->pms->choicemode == CHOICE_FIRSTPASS) {
          (*sbtd->current_button)++;
        } else {
          *sbtd->current_mode = false;
        }
        break;
      }
    }
    if(*sbtd->current_button == sbtd->outkeys->size()) {
      dprintf("Done with pass\n");
      dprintf("%d, %d\n", (*sbtd->outkeys)[0], (*sbtd->outkeys)[1]);
      // Done with the first pass here - this can only happen if the choice if FIRSTPASS
      CHECK(sbtd->pms->choicemode == CHOICE_FIRSTPASS);
      sbtd->pms->settingmode++;
      (*sbtd->current_button) = -1;
    }
  } else {
    CHECK(sbtd->pms->choicemode == CHOICE_ACTIVE);
    if(sbtd->keys.u.repeat)
      (*sbtd->current_button)--;
    if(sbtd->keys.d.repeat)
      (*sbtd->current_button)++;
    if(*sbtd->current_button >= (int)sbtd->outkeys->size())
      *sbtd->current_button = sbtd->outkeys->size() - 1;
    // We accept anything for this (besides cancel) because the user might not know what their accept button is at the moment
    // Maybe add a "done" button at the bottom?
    bool somethingpushed = false;
    for(int i = 0; i < sbtd->keys.keys.size(); i++) {
      if(i == sbtd->pms->buttons[BUTTON_CANCEL])
        continue;
      if(sbtd->keys.keys[i].push)
        somethingpushed = true;
    }
    if(somethingpushed) {
      (*sbtd->current_mode) = true;
    } else if(sbtd->keys.keys[sbtd->pms->buttons[BUTTON_CANCEL]].push || (*sbtd->current_button) == -1) {
      sbtd->pms->choicemode = CHOICE_IDLE;
      (*sbtd->current_button) = -1;
    }
  }
}

void standardButtonRender(const StandardButtonRenderData &sbrd) {
  StackString sstr("standardButtonRender");
  CHECK(sbrd.rin);
  CHECK(sbrd.buttons);
  CHECK(sbrd.names);
  CHECK(sbrd.groups);
  int linesneeded = 0;
  for(int i = 0; i < sbrd.names->size(); i++)
    linesneeded += (*sbrd.names)[i].size();
  {
    set<int> grid(sbrd.groups, sbrd.groups + sbrd.buttons->size());
    assert(grid.size() > 0);
    linesneeded += grid.size() - 1;
    vector<int> grod(sbrd.groups, sbrd.groups + sbrd.buttons->size());
    grod.erase(unique(grod.begin(), grod.end()), grod.end());
    CHECK(grod.size() == grid.size());
  }
  CHECK(linesneeded <= sbrd.rin->ystarts.size() - 1);
  int cy = (sbrd.rin->ystarts.size() - linesneeded + 1) / 2;
  CHECK(cy + linesneeded <= sbrd.rin->ystarts.size());
  for(int i = 0; i < (*sbrd.names).size(); i++) {
    if(sbrd.sel_button == i && !sbrd.sel_button_reading) {
      setColor(C::active_text * sbrd.fadeFactor);
    } else {
      setColor(C::inactive_text * sbrd.fadeFactor);
    }
    if(i && sbrd.groups[i-1] != sbrd.groups[i])
      cy++;
    CHECK(i < sbrd.names->size());
    CHECK(cy < sbrd.rin->ystarts.size());
    for(int j = 0; j < (*sbrd.names)[i].size(); j++)
      drawText((*sbrd.names)[i][j], sbrd.rin->textsize, sbrd.rin->xstart, sbrd.rin->ystarts[cy++]);
    string btext;
    if(sbrd.sel_button == i && sbrd.sel_button_reading) {
      btext = "?";
      setColor(C::active_text * sbrd.fadeFactor);
    } else if((*sbrd.buttons)[i] == -1) {
      btext = "";
    } else {
      if(sbrd.prefixchar == 'B') {
        CHECK(!sbrd.inverts);
        btext = StringPrintf("%c%02d", sbrd.prefixchar, (*sbrd.buttons)[i]);
      } else if(sbrd.prefixchar == 'X') {
        CHECK(sbrd.inverts);
        btext = StringPrintf("%c%d%c", sbrd.prefixchar, (*sbrd.buttons)[i], (*sbrd.inverts)[i] ? '-' : '+');
      } else {
        CHECK(0);
      }
      setColor(C::inactive_text * sbrd.fadeFactor);
    }
    drawJustifiedText(btext.c_str(), sbrd.rin->textsize, sbrd.rin->xend, sbrd.rin->ystarts[cy - 1], TEXT_MAX, TEXT_MIN);
  }
  CHECK(cy <= sbrd.rin->ystarts.size());
}

int GameAiAxisRotater::Randomater::next() {
  if(fleft <= 0) {
    vector<int> opts;
    opts.push_back(1);
    opts.push_back(0);
    opts.push_back(-1);
    opts.erase(find(opts.begin(), opts.end(), current));
    current = opts[int(frand() * 2)];
    fleft = int(frand() * 120 + 120);
    if(current == 0)
      fleft /= 2;
  }
  fleft--;
  return current;
}

GameAiAxisRotater::Randomater::Randomater() {
  current = 0;
  fleft = 0;
}

void GameAiAxisRotater::updateGameWork(const vector<Tank> &players, int me) {
  for(int i = 0; i < 2; i++) {
    if(config.ax[i])
      next[i] = approach(next[i], rands[i].next(), 0.05);
    else
      next[i] = approach(next[i], 0, 0.05);
  }
  
  nextKeys.udlrax.x = next[0];
  nextKeys.udlrax.y = next[1];

  nextKeys.axmode = ax_type;
}

void GameAiAxisRotater::updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
  CHECK(0);
}

void GameAiAxisRotater::updateConfig(const Config &conf) {
  config = conf;
}

GameAiAxisRotater::GameAiAxisRotater(const GameAiAxisRotater::Config &conf, int in_ax_type) {
  rands.resize(2);
  next.resize(2, 0.);
  config = conf;
  ax_type = in_ax_type;
};

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  StackString sstr("runSettingTick");
  if(!pms->faction) { // if player hasn't chosen faction yet
    StackString sstr("chfact");
    pms->fireHeld = 0;
    {
      Float2 dir = deadzone(keys.menu, DEADZONE_CENTER, 0.2) * 0.01;
      dir.y *= -1;
      pms->compasspos += dir;
    }
    {
      Float4 bbx = startFBoundBox();
      for(int j = 0; j < factions.size(); j++)
        addToBoundBox(&bbx, factions[j].compass_location);
      CHECK(bbx.isNormalized());
      if(pms->compasspos.x < bbx.sx)
        pms->compasspos.x = bbx.sx;
      if(pms->compasspos.y < bbx.sy)
        pms->compasspos.y = bbx.sy;
      if(pms->compasspos.x > bbx.ex)
        pms->compasspos.x = bbx.ex;
      if(pms->compasspos.y > bbx.ey)
        pms->compasspos.y = bbx.ey;
    }
    int targetInside = -1;
    for(int j = 0; j < factions.size(); j++) {
      if(isInside(factions[j].compass_location, pms->compasspos) && !factions[j].taken) {
        //CHECK(targetInside == -1);
        targetInside = j;
      }
    }
    if(targetInside != -1) {            
      for(int j = 0; j < keys.keys.size(); j++) {
        if(keys.keys[j].repeat) {
          pms->faction = &factions[targetInside];
          pms->settingmode = SETTING_BUTTONS;
          pms->choicemode = CHOICE_FIRSTPASS; // Because it is.
          factions[targetInside].taken = true;
        }
      }
    }
  } else {
    StackString sstr("proc");
    pms->fireHeld -= 1;
    if(pms->fireHeld < 0)
        pms->fireHeld = 0;
    if(pms->choicemode == CHOICE_IDLE) {
      CHECK(pms->faction);
      if(keys.l.push)
        pms->settingmode--;
      if(keys.r.push)
        pms->settingmode++;
      if(pms->settingmode < 0)
        pms->settingmode = 0;
      if(pms->settingmode >= SETTING_LAST)
        pms->settingmode = SETTING_LAST - 1;
      if((keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.d.down) && pms->settingmode != SETTING_READY)
        pms->choicemode = CHOICE_ACTIVE;
      if((keys.keys[pms->buttons[BUTTON_ACCEPT]].down || keys.d.down) && pms->settingmode == SETTING_READY)
        pms->fireHeld += 2;
      if(pms->fireHeld > 60)
        pms->fireHeld = 60;
    } else if(pms->settingmode == SETTING_BUTTONS) {
      vector<float> triggers;
      for(int i = 0; i < keys.keys.size(); i++)
        triggers.push_back(keys.keys[i].push);
      
      vector<int> groups(BUTTON_LAST);
      groups[BUTTON_ACCEPT] = 1;
      groups[BUTTON_CANCEL] = 1;

      StandardButtonTickData sbtd;      
      sbtd.outkeys = &pms->buttons;
      sbtd.outinvert = NULL;
      sbtd.current_button = &pms->setting_button_current;
      sbtd.current_mode = &pms->setting_button_reading;
      sbtd.groups = button_groups;
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      sbtd.pms = pms;
      
      standardButtonTick(&sbtd);
    } else if(pms->settingmode == SETTING_AXISTYPE && pms->setting_axistype_demo_curframe == -1) {
      bool closing = false;
      
      if(keys.u.push)
        pms->setting_axistype_curchoice--;
      if(keys.d.push)
        pms->setting_axistype_curchoice++;
      if(pms->setting_axistype_curchoice < 0) {
        closing = true;
        pms->setting_axistype_curchoice = 0;
      }
      if(pms->setting_axistype_curchoice >= KSAX_END * 2)
        pms->setting_axistype_curchoice = KSAX_END * 2 - 1;
      
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
        if(pms->setting_axistype_curchoice % 2 == 0) {
          pms->setting_axistype = pms->setting_axistype_curchoice / 2;
        } else {
          pms->setting_axistype_demo_curframe = 0;
          pms->createNewAxistypeDemo();
        }
      }
      
      if(pms->setting_axistype_demo_curframe == -1) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.keys[pms->buttons[BUTTON_CANCEL]].push || closing) {
            pms->setting_axistype_curchoice = 0;
            pms->choicemode = CHOICE_IDLE;
          }
        } else if(pms->choicemode == CHOICE_FIRSTPASS) {
          if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
            pms->setting_axistype_curchoice = 0;
            pms->settingmode++;
          }
        } else {
          CHECK(0);
        }
      }
    } else if(pms->settingmode == SETTING_AXISTYPE && pms->setting_axistype_demo_curframe != -1) {
      
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push)
        pms->setting_axistype_demo_curframe++;
      
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push)
        pms->setting_axistype_demo_curframe = -1;
      
    } else if(pms->settingmode == SETTING_AXISCHOOSE) {
      StackString sstr("SAX");
      vector<float> triggers;
      for(int i = 0; i < keys.axes.size(); i++)
        triggers.push_back(keys.axes[i]);
      
      StandardButtonTickData sbtd;      
      sbtd.outkeys = &pms->axes;
      sbtd.outinvert = &pms->axes_invert;
      sbtd.current_button = &pms->setting_axis_current;
      sbtd.current_mode = &pms->setting_axis_reading;
      sbtd.groups = axis_groups;
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      sbtd.pms = pms;
      
      standardButtonTick(&sbtd);
    } else if(pms->settingmode == SETTING_TEST) {
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push) {
        if(pms->choicemode == CHOICE_FIRSTPASS) {
          pms->settingmode++;
        } else {
          pms->choicemode = CHOICE_IDLE;
        }
      }
    } else if(pms->settingmode == SETTING_READY) {
      pms->choicemode = CHOICE_IDLE; // There is no other option! Idle is the only possibility! Bow down before your god!
    } else {
      CHECK(0);
    }
  }
  
  {
    StackString sstr("hacky");
    
    // this is kind of hacky
    if(pms->settingmode == SETTING_TEST && pms->test_game.empty()) {
      StackString sstr("init");
      CHECK(pms->test_player.empty());
      
      pms->test_player.reset(new Player(pms->faction->faction, 0));
      
      const RenderInfo rin(pms->faction->compass_location);
      
      Float4 boundy = Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]);
      boundy -= boundy.midpoint();
      
      float mn = min(boundy.x_span(), boundy.y_span());
      boundy *= (100 / mn);
      
      pms->test_game.reset(new Game());
      pms->test_game->initTest(pms->test_player.get(), boundy);
    }
    
    // so is this
    if(pms->settingmode == SETTING_TEST) {
      StackString sstr("run");
      if(pms->choicemode == CHOICE_IDLE) {
        vector<Keystates> kst(1);
        CHECK(!pms->test_game->runTick(kst));
      } else if(pms->choicemode == CHOICE_ACTIVE || pms->choicemode == CHOICE_FIRSTPASS) {
        vector<Keystates> kst;
        kst.push_back(genKeystate(keys, *pms));
        CHECK(!pms->test_game->runTick(kst));
      } else {
        CHECK(0);
      }
    }
    
    // oh yeah real hacky now
    if(pms->setting_axistype_demo_curframe != pms->setting_axistype_demo_aiframe) {
      StackString sstr("aiinit");
      int categ;
      if(pms->setting_axistype_curchoice % 2 == 0) {
        CHECK(pms->setting_axistype_demo_curframe == -1);
        categ = -1;
      } else {
        categ = pms->setting_axistype_curchoice / 2;
      }
      
      if(categ == -1) {
        pms->setting_axistype_demo_ai.reset();
      } else if(categ == KSAX_STEERING && pms->setting_axistype_demo_curframe == 0) {
        pms->setting_axistype_demo_ai->updateConfig(GameAiAxisRotater::Config(false, true));
      } else if(categ == KSAX_STEERING && pms->setting_axistype_demo_curframe == 1) {
        pms->setting_axistype_demo_ai->updateConfig(GameAiAxisRotater::Config(true, false));
      } else if(categ == KSAX_STEERING && pms->setting_axistype_demo_curframe == 2) {
        pms->setting_axistype_demo_ai->updateConfig(GameAiAxisRotater::Config(true, true));
      } else if(categ == KSAX_STEERING && pms->setting_axistype_demo_curframe == 3) {
        pms->setting_axistype_demo_curframe = -1;
        pms->setting_axistype_demo_ai.reset();
        pms->setting_axistype_demo_player.reset();
        pms->setting_axistype_demo_game.reset();
      } else {
        CHECK(0);
      }
      pms->setting_axistype_demo_aiframe = pms->setting_axistype_demo_curframe;
    }
    
    // wooooo go hack
    if(!pms->setting_axistype_demo_ai.empty()) {
      StackString sstr("run");
      CHECK(!pms->setting_axistype_demo_game.empty());
      CHECK(!pms->setting_axistype_demo_player.empty());
      
      vector<GameAi *> tai;
      tai.push_back(pms->setting_axistype_demo_ai.get());
  
      pms->setting_axistype_demo_game->ai(tai);
      vector<Keystates> kist;
      for(int i = 0; i < tai.size(); i++)
        kist.push_back(tai[i]->getNextKeys());
      pms->setting_axistype_demo_game->runTick(kist);
    }
  }
}

void runSettingRender(const PlayerMenuState &pms) {
  StackString sstr("runSettingRender");
  if(!pms.faction) {
    setColor(1.0, 1.0, 1.0);
    //char bf[16];
    //sprintf(bf, "p%d", i);
    if(pms.compasspos.x != 0 || pms.compasspos.y != 0) {
      drawLine(pms.compasspos.x, pms.compasspos.y - 0.06, pms.compasspos.x, pms.compasspos.y - 0.02, 0.004);
      drawLine(pms.compasspos.x, pms.compasspos.y + 0.06, pms.compasspos.x, pms.compasspos.y + 0.02, 0.004);
      drawLine(pms.compasspos.x - 0.06, pms.compasspos.y, pms.compasspos.x - 0.02, pms.compasspos.y, 0.004);
      drawLine(pms.compasspos.x + 0.06, pms.compasspos.y, pms.compasspos.x + 0.02, pms.compasspos.y, 0.004);
    }
    //drawText(bf, 20, pms.compasspos.x + 5, pms.compasspos.y + 5);
  } else {
    
    const RenderInfo rin(pms.faction->compass_location);
    const float fadeFactor = (60 - pms.fireHeld) / 60.;
    
    {
      const float nameFade = pow(1.0 - fadeFactor, 2);
      setColor(pms.faction->faction->color * nameFade);
      
      vector<string> text = pms.faction->faction->name_lines;
      text.push_back("");
      text.push_back("Ready");
      
      drawJustifiedMultiText(text, rin.textsize, rin.drawzone.midpoint(), TEXT_CENTER, TEXT_CENTER);
    }
    
    setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
    {
      vector<Float2> rectish;
      rectish.push_back(Float2(rin.drawzone.sx + rin.border, rin.drawzone.sy));
      rectish.push_back(Float2(rin.drawzone.ex - rin.border, rin.drawzone.sy));
      rectish.push_back(Float2(rin.drawzone.ex, rin.drawzone.sy + rin.border));
      rectish.push_back(Float2(rin.drawzone.ex, rin.drawzone.ey - rin.border));
      rectish.push_back(Float2(rin.drawzone.ex - rin.border, rin.drawzone.ey));
      rectish.push_back(Float2(rin.drawzone.sx + rin.border, rin.drawzone.ey));
      rectish.push_back(Float2(rin.drawzone.sx, rin.drawzone.ey - rin.border));
      rectish.push_back(Float2(rin.drawzone.sx, rin.drawzone.sy + rin.border));
      drawLineLoop(rectish, 0.003);
    }
    
    setColor(Color(0.3, 0.3, 0.3) * fadeFactor);
    drawLine(Float4(rin.drawzone.sx, (rin.ystarts[0] + rin.textsize + rin.ystarts[1]) / 2, rin.drawzone.ex, (rin.ystarts[0] + rin.textsize + rin.ystarts[1]) / 2), 0.001);
    
    {
      // Topic line!
      setColor(pms.faction->faction->color * fadeFactor);
      drawDvec2(pms.faction->faction->icon, Float4(rin.xstart, rin.ystarts[0], rin.xstart + rin.textsize, rin.ystarts[0] + rin.textsize), 50, 0.003);

      float txstart = rin.xstart + rin.textsize + rin.border * 2;
	
      if(pms.choicemode == CHOICE_FIRSTPASS) {
        setColor(C::active_text * fadeFactor);
        drawJustifiedText(setting_names_detailed[pms.settingmode], rin.textsize, (rin.drawzone.sx + rin.drawzone.ex) / 2, rin.ystarts[0], TEXT_CENTER, TEXT_MIN);
      } else {
        const int activescale = 4;
        float title_units = (rin.xend - txstart) / (SETTING_LAST - 1 + activescale);
        
        int units = 0;
      
        for(int i = 0; i < SETTING_LAST; i++) {
          bool active = (i == pms.settingmode);
          
          int tunits = active ? activescale : 1;
          string text = active ? setting_names[i] : string() + setting_names[i][0];
          setColor(((active && pms.choicemode == CHOICE_IDLE) ? C::active_text : C::inactive_text) * fadeFactor);
          
          drawJustifiedText(text, rin.textsize, title_units * (units + tunits / 2.) + txstart, rin.ystarts[0], TEXT_CENTER, TEXT_MIN);
          
          units += tunits;
        }
        
        CHECK(units == SETTING_LAST - 1 + activescale);
      }
      
    }
    
    if(pms.settingmode == SETTING_BUTTONS) {
      vector<vector<string> > names;
      for(int i = 0; i < BUTTON_LAST; i++) {
        vector<string> tix;
        tix.push_back(button_names[i]);
        names.push_back(tix);
      }
      
      StandardButtonRenderData sbrd;
      sbrd.rin = &rin;
      sbrd.buttons = &pms.buttons;
      sbrd.inverts = NULL;
      sbrd.names = &names;
      sbrd.groups = button_groups;
      sbrd.sel_button = pms.setting_button_current;
      sbrd.sel_button_reading = pms.setting_button_reading;
      sbrd.fadeFactor = fadeFactor;
      sbrd.prefixchar = 'B';
      
      standardButtonRender(sbrd);
    } else if(pms.settingmode == SETTING_AXISTYPE && pms.setting_axistype_demo_curframe == -1) {
      for(int i = 0; i < KSAX_END * 2; i++) {
        if(pms.choicemode != CHOICE_IDLE && pms.setting_axistype_curchoice == i)
          setColor(C::active_text * fadeFactor);
        else
          setColor(C::inactive_text * fadeFactor);
        
        if(i % 2 == 0)
          drawText(ksax_names[i / 2], rin.textsize, rin.xstart + rin.textsize * 2, rin.ystarts[i + 2]);
        else
          drawText("(demo)", rin.textsize, rin.xstart + rin.textsize * 3, rin.ystarts[i + 2]);
      }
      
      if(pms.setting_axistype != -1) {
        setColor(C::active_text * fadeFactor);
        drawText(">", rin.textsize, rin.xstart, rin.ystarts[pms.setting_axistype * 2 + 2]);
      }
    } else if(pms.settingmode == SETTING_AXISTYPE && pms.setting_axistype_demo_curframe != -1) {
      
      CHECK(!pms.setting_axistype_demo_game.empty());
      {
        GfxWindow gfxw(Float4(rin.xend - (rin.ystarts[7] - rin.ystarts[1]), rin.ystarts[1], rin.xend, rin.ystarts[7]), fadeFactor);
        pms.setting_axistype_demo_game->renderToScreen();
      }
      
      setColor(C::inactive_text * fadeFactor);
      if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_curframe == 0) {
        drawText("Move controller", rin.textsize, rin.xstart, rin.ystarts[1]);
        drawText("forward and", rin.textsize, rin.xstart, rin.ystarts[2]);
        drawText("back to drive", rin.textsize, rin.xstart, rin.ystarts[3]);
        drawText("forward and", rin.textsize, rin.xstart, rin.ystarts[4]);
        drawText("back", rin.textsize, rin.xstart, rin.ystarts[5]);
      } else if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_curframe == 1) {      
        drawText("Move controller", rin.textsize, rin.xstart, rin.ystarts[1]);
        drawText("side to side", rin.textsize, rin.xstart, rin.ystarts[2]);
        drawText("to turn", rin.textsize, rin.xstart, rin.ystarts[3]);
      } else if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_curframe == 2) {      
        drawText("Combine these", rin.textsize, rin.xstart, rin.ystarts[1]);
        drawText("to drive around", rin.textsize, rin.xstart, rin.ystarts[2]);
      } else {
        CHECK(0);
      }
      
      setColor(C::active_text * fadeFactor);
      if(pms.setting_axistype_demo_curframe != 2) {
        drawJustifiedText("Push accept to continue", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      } else {
        drawJustifiedText("Push accept to return", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      }
      
    } else if(pms.settingmode == SETTING_AXISCHOOSE) {
      StandardButtonRenderData sbrd;
      sbrd.rin = &rin;
      sbrd.buttons = &pms.axes;
      sbrd.inverts = &pms.axes_invert;
      sbrd.names = &ksax_axis_names[pms.setting_axistype];
      sbrd.groups = axis_groups;
      sbrd.sel_button = pms.setting_axis_current;
      sbrd.sel_button_reading = pms.setting_axis_reading;
      sbrd.fadeFactor = fadeFactor;
      sbrd.prefixchar = 'X';
      
      standardButtonRender(sbrd);
    } else if(pms.settingmode == SETTING_TEST) {
      setColor(C::inactive_text * fadeFactor);
      if(pms.choicemode == CHOICE_IDLE) {
        drawJustifiedText("Push accept to test", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      } else {
        drawJustifiedText("Push cancel when done", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      }
      GfxWindow gfxw(Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]), fadeFactor);
      pms.test_game->renderToScreen();
    } else if(pms.settingmode == SETTING_READY) {
      setColor(C::inactive_text * fadeFactor);
      const char * const text[] = {"Move left/right over", "K-M-D-T to change options.", "", "Hold accept when", "ready to start. Let", "go of button to cancel."};
      for(int i = 0; i < 6; i++)
        drawJustifiedText(text[i], rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[i + 1], TEXT_CENTER, TEXT_MIN);
    } else {
      CHECK(0);
    }
  }
}
