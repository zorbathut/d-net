
#include "metagame_config.h"

#include "debug.h"
#include "game.h"
#include "game_tank.h"
#include "gfx.h"
#include "player.h"

using namespace std;

class RenderInfo {
public:
  // Basic math!
  static const float textline_size;  // How many times larger a line of text is than its divider
  static const float border_size;
  static const float top_border_extra_size;
  static const float divider_size;	// Division between tabs and content
  static const float units;

  static const int textline_count = 16; // How many lines we're going to have
  static const float linethick;

  // runtime constants
  Float4 drawzone;
  float unitsize;
  float border;
  float external_xstart;
  float external_xend;
  float xstart;
  float xend;
  vector<float> ystarts;

  float textsize;
  float textborder;

  float tab_top;
  float tab_bottom;
  float expansion_bottom;

  float aspect;
  
  RenderInfo() {
    aspect = 1.532563;
    
    const float roundaborder = 0.05;
    
    drawzone = Float4(roundaborder, roundaborder, aspect - roundaborder, 1.0 - roundaborder);     // maaaagic

    // runtime constants
    unitsize = drawzone.y_span() / units;
    border = unitsize * border_size;
    external_xstart = drawzone.sx + border;
    external_xend = drawzone.ex - border;
    ystarts.resize(textline_count);
    ystarts[0] = drawzone.sy + unitsize * top_border_extra_size;
    textsize = unitsize * textline_size;
    xstart = external_xstart + textsize * 2;
    xend = external_xend - textsize * 2;
    for(int i = 1; i < textline_count; i++)
      ystarts[i] = ystarts[0] + unitsize * (divider_size + i - 1) + textsize * i;
    textborder = textsize / textline_size;
    
    tab_top = drawzone.sy;
    tab_bottom = ystarts[0] + textsize + border;
    expansion_bottom = ystarts[1] - border;
  }
};

// compiletime constants. All values are in multiples of Divider.
const float RenderInfo::textline_size = 3;  // How many times larger a line of text is than its divider
const float RenderInfo::border_size = 1.5;
const float RenderInfo::top_border_extra_size = 1.5;
const float RenderInfo::divider_size = 5;	// Division between tabs and content in dividers
const float RenderInfo::units = textline_size * textline_count + textline_count - 1 + border_size + top_border_extra_size + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders, plus the increased divider from categories to data
const float RenderInfo::linethick = 0.003;

PlayerMenuState::PlayerMenuState() {
  settingmode = SETTING_BUTTONS;
  choicemode = CHOICE_FIRSTPASS;
  
  setting_button_current = -1;
  setting_axis_current = -1;
  
  setting_axistype = -1;
  setting_old_axistype = -1;
  
  setting_axistype_curchoice = 0;
  setting_axistype_demo_cursegment = -1;
  
  buttons.resize(BUTTON_LAST, -1);
  axes.resize(2, -1);
  axes_invert.resize(axes.size(), false);
  
  faction = NULL;
  compasspos = Float2(0, 0);
}

PlayerMenuState::~PlayerMenuState() { }

void PlayerMenuState::createNewAxistypeDemo() {
  setting_axistype_demo.reset();
  
  setting_axistype_demo.reset(new GamePackage);
  
  setting_axistype_demo->players.push_back(Player(faction->faction, 0));
  
  const RenderInfo rin;
  
  Float4 boundy = Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]);
  boundy -= boundy.midpoint();
  
  float mn = min(boundy.x_span(), boundy.y_span());
  boundy *= (100 / mn);
  
  setting_axistype_demo->game.initCenteredDemo(&setting_axistype_demo->players[0], 50);
  
  setting_axistype_demo_ai.reset(new GameAiAxisRotater(GameAiAxisRotater::steeringConfig(false, false)));
}

Keystates PlayerMenuState::genKeystate(const Controller &keys) const {
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  CHECK(SIMUL_WEAPONS == 2);
  kst.accept = keys.keys[buttons[BUTTON_ACCEPT]];
  kst.cancel = keys.keys[buttons[BUTTON_CANCEL]];
  kst.fire[0] = keys.keys[buttons[BUTTON_FIRE1]];
  kst.fire[1] = keys.keys[buttons[BUTTON_FIRE2]];
  kst.change[0] = keys.keys[buttons[BUTTON_SWITCH1]];
  kst.change[1] = keys.keys[buttons[BUTTON_SWITCH2]];
  kst.axmode = setting_axistype;
  for(int j = 0; j < 2; j++) {
    kst.ax[j] = keys.axes[axes[j]];
    if(axes_invert[j])
      kst.ax[j] *= -1;
  }
  kst.udlrax = keys.menu;
  CHECK(keys.menu.x >= -1 && keys.menu.x <= 1);
  CHECK(keys.menu.y >= -1 && keys.menu.y <= 1);
  return kst;
}

struct StandardButtonTickData {
  vector<int> *outkeys;
  vector<char> *outinvert;
  
  int *current_button;
  ReadMode *current_mode;

  bool require_trigger;
  int accept_button;
  int cancel_button;
  
  const int *groups;
  
  Controller keys;
  const vector<float> *triggers;
};

struct StandardButtonRenderData {
  const RenderInfo *rin;
  
  const vector<int> *buttons;
  const vector<char> *inverts;
  
  const vector<vector<string> > *names;
  const vector<string> *groupnames;
  const int *groups;
  
  int sel_button;
  bool sel_button_reading;
  
  string prefix;
  vector<string> description;
};

bool changeButtons(vector<int> *buttons, vector<char> *inversions, const int *groups, int choice, int button, bool inverted) {
  // Step 1: If this button has been chosen somewhere else, and we're not choosing this button for the first time, swap 'em (TODO: go back?)
  for(int j = 0; j < buttons->size(); j++) {
    if(groups[j] != groups[choice])
      continue;
    if((*buttons)[j] == button) {
      if((*buttons)[choice] == -1) {
        return false;
      } else {
        swap((*buttons)[j], (*buttons)[choice]);
        if(inversions) {
          swap((*inversions)[j], (*inversions)[choice]);
          (*inversions)[choice] = inverted;
        }
        return true;
      }
      CHECK(0);
    }
  }
  
  // If we got to this point, our button is being chosen for the first time.
  (*buttons)[choice] = button;
  if(inversions)
    (*inversions)[choice] = inverted;
  
  return true;
}

bool standardButtonTick(StandardButtonTickData *sbtd) {
  StackString sstr("standardButtonTick");
  CHECK(sbtd->outkeys);
  CHECK(sbtd->outkeys->size());
  if(sbtd->outinvert)
    CHECK(sbtd->outkeys->size() == sbtd->outinvert->size());
  if(*sbtd->current_button == -1) {
    if((*sbtd->outkeys)[0] == -1) {
      *sbtd->current_mode = RM_SHUNTING;
    } else {
      *sbtd->current_mode = RM_IDLE;
    }
    *sbtd->current_button = 0;
  }
  CHECK(*sbtd->current_button >= 0 && *sbtd->current_button <= sbtd->outkeys->size());
  if(*sbtd->current_mode != RM_IDLE) {
    StackString sstr("rmidle");
    for(int i = 0; i < sbtd->triggers->size(); i++) { // For each input button . . .
      if(abs((*sbtd->triggers)[i]) > 0.9) { // If button was pushed . . .
        if(changeButtons(sbtd->outkeys, sbtd->outinvert, sbtd->groups, *sbtd->current_button, i, (*sbtd->triggers)[i] < 0)) { // If button successfully changed . . .
          if(*sbtd->current_mode == RM_SHUNTING) {
            (*sbtd->current_button)++;
          } else if(*sbtd->current_mode == RM_CHOOSING) {
            *sbtd->current_mode = RM_IDLE;
          } else {
            CHECK(0);
          }
          break;
        }
      }
    }
  } else {
    StackString sstr("notidle");
    if(sbtd->keys.u.repeat)
      (*sbtd->current_button)--;
    if(sbtd->keys.d.repeat)
      (*sbtd->current_button)++;
    *sbtd->current_button = modurot(*sbtd->current_button, sbtd->outkeys->size() + 1);
    
    if(*sbtd->current_button == sbtd->outkeys->size()) {
      if(sbtd->keys.keys[sbtd->accept_button].push) {
        *sbtd->current_button = -1;
        return true;
      }
    }
    
    if(sbtd->require_trigger) {
      // We accept anything for this (besides cancel) because the user might not know what their accept button is at the moment
      bool somethingpushed = false;
      for(int i = 0; i < sbtd->keys.keys.size(); i++) {
        if(i == sbtd->cancel_button)
          continue;
        if(sbtd->keys.keys[i].push)
          somethingpushed = true;
      }
      if(somethingpushed) {
        (*sbtd->current_mode) = RM_CHOOSING;
      } else if(sbtd->keys.keys[sbtd->cancel_button].push) {
        *sbtd->current_button = -1;
        return true;
      }
    } else {
      for(int i = 0; i < sbtd->triggers->size(); i++) {
        if(abs((*sbtd->triggers)[i]) > 0.9) {
          changeButtons(sbtd->outkeys, sbtd->outinvert, sbtd->groups, *sbtd->current_button, i, (*sbtd->triggers)[i] < 0);
        }
      }
    }
  }
  
  if(*sbtd->current_button == sbtd->outkeys->size()) {
    *sbtd->current_mode = RM_IDLE;
  }
  
  return false;
}

void drawBottomBlock(const RenderInfo &rin, int lines) {
  float bottom_point = rin.ystarts[rin.textline_count - lines] - getTextBoxBorder(rin.textsize);
  drawSolid(Float4(rin.drawzone.sx, bottom_point, rin.drawzone.ex, rin.drawzone.ey));
  setColor(C::box_border);
  drawLine(Float4(rin.drawzone.sx, bottom_point, rin.drawzone.ex, bottom_point), getTextBoxThickness(rin.textsize));
}

void standardButtonRender(const StandardButtonRenderData &sbrd) {
  StackString sstr("standardButtonRender");
  CHECK(sbrd.rin);
  CHECK(sbrd.buttons);
  CHECK(sbrd.names);
  CHECK(sbrd.groups);
  int linesneeded = 2;
  for(int i = 0; i < sbrd.names->size(); i++)
    linesneeded += (*sbrd.names)[i].size();
  
  {
    set<int> grid(sbrd.groups, sbrd.groups + sbrd.buttons->size());
    assert(grid.size() > 0);
    linesneeded += grid.size() - 1;
    
    vector<int> grod(sbrd.groups, sbrd.groups + sbrd.buttons->size());
    grod.erase(unique(grod.begin(), grod.end()), grod.end());
    CHECK(grod.size() == grid.size());
    
    if(sbrd.groupnames)
      linesneeded += grid.size();
  }
  float xps = sbrd.rin->xstart;
  float groupnamexps = xps;
  if(sbrd.groupnames)
    xps += sbrd.rin->textsize;
  CHECK(linesneeded <= sbrd.rin->ystarts.size() - 1 - sbrd.description.size());
  int scy = (sbrd.rin->ystarts.size() - linesneeded - sbrd.description.size() + 1) / 2;
  int cy = scy;
  CHECK(cy + linesneeded <= sbrd.rin->ystarts.size());
  drawTextBoxAround(Float4(sbrd.rin->xstart, sbrd.rin->ystarts[scy], sbrd.rin->xend, sbrd.rin->ystarts[scy + linesneeded - 1] + sbrd.rin->textsize), sbrd.rin->textsize);
  for(int i = 0; i < (*sbrd.names).size(); i++) {\
    if(i && sbrd.groups[i-1] != sbrd.groups[i])
      cy++;
    if(sbrd.groupnames) {
      if(!i || sbrd.groups[i-1] != sbrd.groups[i])  {
        setColor(C::inactive_text);
        drawText((*sbrd.groupnames)[sbrd.groups[i]], sbrd.rin->textsize, Float2(groupnamexps, sbrd.rin->ystarts[cy++]));
      }
    }
    if(sbrd.sel_button == i && !sbrd.sel_button_reading) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    CHECK(i < sbrd.names->size());
    CHECK(cy < sbrd.rin->ystarts.size());
    for(int j = 0; j < (*sbrd.names)[i].size(); j++)
      drawText((*sbrd.names)[i][j], sbrd.rin->textsize, Float2(xps, sbrd.rin->ystarts[cy++]));
    string btext;
    if(sbrd.sel_button == i && sbrd.sel_button_reading) {
      btext = "?";
      setColor(C::active_text);
    } else if((*sbrd.buttons)[i] == -1) {
      btext = "";
    } else {
      if(!sbrd.inverts) {
        btext = StringPrintf("%s%d", sbrd.prefix.c_str(), (*sbrd.buttons)[i]);
      } else {
        btext = StringPrintf("%s%d%c", sbrd.prefix.c_str(), (*sbrd.buttons)[i], (*sbrd.inverts)[i] ? '-' : '+');
      }
      setColor(C::inactive_text);
    }
    drawJustifiedText(btext.c_str(), sbrd.rin->textsize, Float2(sbrd.rin->xend, sbrd.rin->ystarts[cy - 1]), TEXT_MAX, TEXT_MIN);
  }
  cy++;
  if(sbrd.sel_button == sbrd.names->size()) {
    setColor(C::active_text);
  } else {
    setColor(C::inactive_text);
  }
  drawText("Done", sbrd.rin->textsize, Float2(groupnamexps, sbrd.rin->ystarts[cy++]));
  CHECK(cy == scy + linesneeded);

  drawBottomBlock(*sbrd.rin, sbrd.description.size());
  setColor(C::inactive_text);
  for(int i = 0; i < sbrd.description.size(); i++)
    drawJustifiedText(sbrd.description[i], sbrd.rin->textsize, Float2((sbrd.rin->xstart + sbrd.rin->xend) / 2, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - sbrd.description.size() + i]), TEXT_CENTER, TEXT_MIN);
}

float GameAiAxisRotater::Randomater::next() {
  if(fleft <= 0) {
    if(smooth) {
      current = frand() * 2 - 1;
    } else {
      vector<float> opts;
      opts.push_back(1);
      opts.push_back(0);
      opts.push_back(-1);
      opts.erase(find(opts.begin(), opts.end(), current));
      current = opts[int(frand() * opts.size())];
    }
    fleft = int(frand() * 120 + 120);
    if(!smooth) {
      if(current == 0)
        fleft /= 2;
    }
  }
  fleft--;
  return current;
}

GameAiAxisRotater::Randomater::Randomater() {
  current = 0;
  fleft = 0;
}

void GameAiAxisRotater::updateGameWork(const vector<Tank> &players, int me) {
  StackString sstr("udg");
  if(config.type == KSAX_STEERING) {
    CHECK(rands.size() == 2);
    for(int i = 0; i < 2; i++) {
      if(config.ax[i])
        next[i] = approach(next[i], rands[i].next(), 0.05);
      else
        next[i] = approach(next[i], 0, 0.05);
    }
  } else if(config.type == KSAX_ABSOLUTE) {
    CHECK(rands.size() == 1);
    Float2 ps = makeAngle(rands[0].next() * PI);
    next[0] = approach(next[0], ps.x, 0.05);
    next[1] = approach(next[1], ps.y, 0.05);
  } else if(config.type == KSAX_TANK) {
    CHECK(rands.size() == 2);
    for(int i = 0; i < 2; i++) {
      if(config.tax[i] == -1)
        next[i] = approach(next[i], 0, 0.05);
      else
        next[i] = approach(next[i], rands[config.tax[i]].next(), 0.05);
    }
  } else {
    CHECK(0);
  }
  
  nextKeys.udlrax = getControls();

  nextKeys.axmode = config.type;
}

void GameAiAxisRotater::updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
  CHECK(0);
}

void GameAiAxisRotater::updateConfig(const Config &conf) {
  config = conf;
  if(config.type == KSAX_STEERING || config.type == KSAX_TANK) {
    rands.resize(2);
    rands[0].smooth = false;
    rands[1].smooth = false;
  } else if(config.type == KSAX_ABSOLUTE) {
    rands.resize(1);
    rands[0].smooth = true;
  } else {
    CHECK(0);
  }
}

GameAiAxisRotater::Config GameAiAxisRotater::steeringConfig(bool ax0, bool ax1) {
  Config conf;
  conf.type = KSAX_STEERING;
  conf.ax[0] = ax0;
  conf.ax[1] = ax1;
  return conf;
}
GameAiAxisRotater::Config GameAiAxisRotater::absoluteConfig() {
  Config conf;
  conf.type = KSAX_ABSOLUTE;
  return conf;
}
GameAiAxisRotater::Config GameAiAxisRotater::tankConfig(int axlsrc, int axrsrc) {
  Config conf;
  conf.type = KSAX_TANK;
  conf.tax[0] = axlsrc;
  conf.tax[1] = axrsrc;
  for(int i = 0; i < 2; i++)
    CHECK(conf.tax[i] >= -1 && conf.tax[i] < 2);
  return conf;
}

Float2 GameAiAxisRotater::getControls() const {
  return Float2(next[0], next[1]);
}

GameAiAxisRotater::GameAiAxisRotater(const GameAiAxisRotater::Config &conf) {
  next.resize(2, 0.);
  updateConfig(conf);
}

vector<pair<float, float> > choiceTopicXpos(float sx, float ex, float textsize) {
  vector<pair<float, float> > rv;
  float jtx = 0;
  for(int i = 0; i < SETTING_LAST; i++) {
    
    float sps = jtx;
    jtx += getTextWidth(setting_names[i], textsize);
    rv.push_back(make_pair(sps, jtx));
    jtx += textsize * 4;
  }
  
  float wid = rv.back().second;
  if(wid > ex - sx)
    dprintf("%f, %f\n", wid, ex - sx);
  CHECK(wid <= ex - sx);
  float shift = (ex - sx - wid) / 2 + sx;
  for(int i = 0; i < rv.size(); i++) {
    rv[i].first += shift;
    rv[i].second += shift;
  }
  
  return rv;
}

int configDemoSegments(int type) {
  if(type == KSAX_STEERING) {
    return 3;
  } else if(type == KSAX_ABSOLUTE) {
    return 2;
  } else if(type == KSAX_TANK) {
    return 4;
  } else {
    CHECK(0);
  }
}
vector<string> configDemoText(int type, int segment);
GameAiAxisRotater::Config configDemoRotater(int type, int segment) {
  if(type == KSAX_STEERING && segment == 0) {
    return GameAiAxisRotater::steeringConfig(false, true);
  } else if(type == KSAX_STEERING && segment == 1) {
    return GameAiAxisRotater::steeringConfig(true, false);
  } else if(type == KSAX_STEERING && segment == 2) {
    return GameAiAxisRotater::steeringConfig(true, true);
  } else if(type == KSAX_ABSOLUTE && segment == 0) {
    return GameAiAxisRotater::absoluteConfig();
  } else if(type == KSAX_ABSOLUTE && segment == 1) {
    return GameAiAxisRotater::absoluteConfig();
  } else if(type == KSAX_TANK && segment == 0) {
    return GameAiAxisRotater::tankConfig(0, -1);
  } else if(type == KSAX_TANK && segment == 1) {
    return GameAiAxisRotater::tankConfig(-1, 1);
  } else if(type == KSAX_TANK && segment == 2) {
    return GameAiAxisRotater::tankConfig(1, 1);
  } else if(type == KSAX_TANK && segment == 3) {
    return GameAiAxisRotater::tankConfig(0, 1);
  } else {
    CHECK(0);
  }
}

bool runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  StackString sstr("runSettingTick");
  if(!pms->faction) { // if player hasn't chosen faction yet
    StackString sstr("chfact");
    {
      Float2 dir = deadzone(keys.menu, DEADZONE_CENTER, 0.2) * 0.02;
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
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push && pms->settingmode == SETTING_READY)
        return true;
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push)
        pms->choicemode = CHOICE_ACTIVE;
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
      sbtd.require_trigger = false;
      sbtd.accept_button = pms->buttons[BUTTON_ACCEPT];
      sbtd.cancel_button = pms->buttons[BUTTON_CANCEL];
      sbtd.groups = button_groups;
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      
      if(standardButtonTick(&sbtd)) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          pms->choicemode = CHOICE_IDLE;
        } else if(pms->choicemode == CHOICE_FIRSTPASS) {
          pms->settingmode++;
        } else {
          CHECK(0);
        }
      }
    } else if(pms->settingmode == SETTING_AXISTYPE && pms->setting_axistype_demo_cursegment == -1) {
      bool closing = false;
      
      if(keys.u.push)
        pms->setting_axistype_curchoice--;
      if(keys.d.push)
        pms->setting_axistype_curchoice++;
      
      pms->setting_axistype_curchoice = modurot(pms->setting_axistype_curchoice, KSAX_END * 2 + 1);
      
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
        if(pms->setting_axistype_curchoice == KSAX_END * 2) {
          closing = true;
        } else if(pms->setting_axistype_curchoice % 2 == 0) {
          pms->setting_axistype = pms->setting_axistype_curchoice / 2;
        } else {
          pms->setting_axistype_demo_cursegment = 0;
          pms->createNewAxistypeDemo();
        }
      }
      
      if(pms->setting_axistype_demo_cursegment == -1) { // this only fails if we've gone into a demo mode
        if(closing) {
          if(pms->setting_old_axistype != pms->setting_axistype) {
            pms->setting_old_axistype = pms->setting_axistype;
            pms->setting_axistype_curchoice = 0;
            pms->settingmode++;
            if(pms->choicemode == CHOICE_ACTIVE) {
              pms->choicemode = CHOICE_REAXIS;
              pms->axes.clear();
              pms->axes.resize(2, -1);
            }
          } else {
            if(pms->choicemode != CHOICE_FIRSTPASS) {
              pms->setting_axistype_curchoice = 0;
              pms->choicemode = CHOICE_IDLE;
            }
          }
        }
      }
    } else if(pms->settingmode == SETTING_AXISTYPE && pms->setting_axistype_demo_cursegment != -1) {
      
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push)
        pms->setting_axistype_demo_cursegment++;
      
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push)
        pms->setting_axistype_demo_cursegment = -1;
      
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
      sbtd.require_trigger = true;
      sbtd.accept_button = pms->buttons[BUTTON_ACCEPT];
      sbtd.cancel_button = pms->buttons[BUTTON_CANCEL];
      sbtd.groups = axis_groups;
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      
      if(standardButtonTick(&sbtd)) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          pms->choicemode = CHOICE_IDLE;
        } else if(pms->choicemode == CHOICE_FIRSTPASS || pms->choicemode == CHOICE_REAXIS) {
          pms->settingmode++;
        } else {
          CHECK(0);
        }
      }
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
      
      pms->test_game.reset(new GamePackage);
      
      pms->test_game->players.push_back(Player(pms->faction->faction, 0));
      
      const RenderInfo rin;
      
      Float4 boundy = Float4(rin.xstart, rin.ystarts[2], rin.xend, rin.ystarts[rin.textline_count - 4]);
      boundy -= boundy.midpoint();
      
      float mn = min(boundy.x_span(), boundy.y_span());
      boundy *= (100 / mn);
      
      pms->test_game->game.initTest(&pms->test_game->players[0], boundy);
    }
    
    // so is this
    if(pms->settingmode == SETTING_TEST) {
      StackString sstr("runtest");
      if(pms->choicemode == CHOICE_IDLE) {
        vector<Keystates> kst(1);
        CHECK(!pms->test_game->runTick(kst));
      } else if(pms->choicemode == CHOICE_ACTIVE || pms->choicemode == CHOICE_FIRSTPASS || pms->choicemode == CHOICE_REAXIS) {
        vector<Keystates> kst;
        kst.push_back(pms->genKeystate(keys));
        CHECK(!pms->test_game->runTick(kst));
      } else {
        CHECK(0);
      }
    }
    
    // oh yeah real hacky now
    if(pms->setting_axistype_demo_cursegment != pms->setting_axistype_demo_aiframe) {
      StackString sstr("aiinit");
      int categ;
      if(pms->setting_axistype_curchoice % 2 == 0) {
        CHECK(pms->setting_axistype_demo_cursegment == -1);
        categ = -1;
      } else {
        categ = pms->setting_axistype_curchoice / 2;
      }
      
      if(categ == -1) {
        pms->setting_axistype_demo_ai.reset();
      } else if(pms->setting_axistype_demo_cursegment == -1) {
      } else if(configDemoSegments(categ) == pms->setting_axistype_demo_cursegment) {
        pms->setting_axistype_demo_cursegment = -1;
      } else {
        pms->setting_axistype_demo_ai->updateConfig(configDemoRotater(categ, pms->setting_axistype_demo_cursegment));
      }
      
      if(pms->setting_axistype_demo_cursegment == -1) {
        pms->setting_axistype_demo_ai.reset();
        pms->setting_axistype_demo.reset();
      }
      
      pms->setting_axistype_demo_aiframe = pms->setting_axistype_demo_cursegment;
    }
    
    // wooooo go hack
    if(!pms->setting_axistype_demo_ai.empty()) {
      StackString sstr("rundemo");
      CHECK(!pms->setting_axistype_demo.empty());
  
      vector<Keystates> kist;      
      
      {
        StackString sstr("ai");
        vector<GameAi *> tai;
        tai.push_back(pms->setting_axistype_demo_ai.get());
        pms->setting_axistype_demo->game.ai(tai);
        for(int i = 0; i < tai.size(); i++)
          kist.push_back(tai[i]->getNextKeys());
      }
  
      {
        StackString sstr("game");
        pms->setting_axistype_demo->runTick(kist);
      }
    }
  }
  
  return false;
}

void runSettingRender(const PlayerMenuState &pms, const string &availdescr) {
  StackString sstr("runSettingRender");
  CHECK(pms.faction);

  setZoomVertical(0, 0, 1);
  const RenderInfo rin;
  
  CHECK(abs((getAspect() / rin.aspect) - 1.0) < 0.0001);
  
  // First we render the logo in the center
  {
    CHECK(pms.faction);
    CHECK(pms.faction->faction);
    setColor(pms.faction->faction->color * 0.5);
    drawDvec2(pms.faction->faction->icon, squareInside(Float4(rin.external_xstart, rin.ystarts[2], rin.external_xend, rin.ystarts[rin.textline_count - 1])), 20, 0.01);
  }

  // Then we render our context-sensitive stuff, so it can be rendered over the logo as appropriate.
  if(pms.settingmode == SETTING_BUTTONS) {
    vector<vector<string> > names;
    for(int i = 0; i < BUTTON_LAST; i++) {
      vector<string> tix;
      tix.push_back(button_names[i]);
      names.push_back(tix);
    }
    
    vector<string> groups;
    groups.push_back("Weapon buttons");
    groups.push_back("Menu buttons");
    
    StandardButtonRenderData sbrd;
    sbrd.rin = &rin;
    sbrd.buttons = &pms.buttons;
    sbrd.inverts = NULL;
    sbrd.names = &names;
    sbrd.groupnames = &groups;
    sbrd.groups = button_groups;
    sbrd.sel_button = pms.setting_button_current;
    sbrd.sel_button_reading = pms.setting_button_reading;
    sbrd.prefix = "Button #";
    sbrd.description.push_back("Select your button setup. Choose \"done\" when ready.");
    if(availdescr.size())
      sbrd.description.push_back(availdescr);
    
    standardButtonRender(sbrd);
  } else if(pms.settingmode == SETTING_AXISTYPE && pms.setting_axistype_demo_cursegment == -1) {
    int stopos = (rin.textline_count - 1 - KSAX_END * 2 - 2 - 3) / 2 + 1;
    drawTextBoxAround(Float4(rin.xstart, rin.ystarts[stopos], rin.xend, rin.ystarts[stopos + KSAX_END * 2 + 1] + rin.textsize), rin.textsize);
    for(int i = 0; i < KSAX_END * 2 + 1; i++) {
      if(pms.choicemode != CHOICE_IDLE && pms.setting_axistype_curchoice == i)
        setColor(C::active_text);
      else
        setColor(C::inactive_text);
      
      if(i == KSAX_END * 2)
        drawText("Done", rin.textsize, Float2(rin.xstart, rin.ystarts[i + stopos + 1]));
      else if(i % 2 == 0)
        drawText(ksax_names[i / 2], rin.textsize, Float2(rin.xstart, rin.ystarts[i + stopos]));
      else
        drawText("(demo)", rin.textsize, Float2(rin.xstart + rin.textsize, rin.ystarts[i + stopos]));
    }
    
    
    if(pms.setting_axistype != -1) {
      setColor(C::active_text);
      drawText(">", rin.textsize, Float2(rin.xstart - rin.textsize, rin.ystarts[pms.setting_axistype * 2 + stopos]));
    }
    
    drawBottomBlock(rin, 3);
    setColor(C::inactive_text);
    drawJustifiedText("Choose your tank control mode. Choose \"done\" when", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.ystarts.size() - 3]), TEXT_CENTER, TEXT_MIN);
    drawJustifiedText("ready. Choose \"demo\" for a demonstration of that", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.ystarts.size() - 2]), TEXT_CENTER, TEXT_MIN);
    drawJustifiedText("mode. If you're unsure, \"Steering\" is recommended.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.ystarts.size() - 1]), TEXT_CENTER, TEXT_MIN);
  } else if(pms.settingmode == SETTING_AXISTYPE && pms.setting_axistype_demo_cursegment != -1) {
    
    const float demowindowwidth = rin.ystarts[6] - rin.ystarts[1] + rin.textsize;
    const Float4 demowindow = Float4(rin.xend - demowindowwidth, rin.ystarts[1], rin.xend, rin.ystarts[1] + demowindowwidth);
    const float controllerwindowwidth = rin.ystarts[6] - rin.ystarts[5] + rin.textsize;
    const Float4 controllerwindow = Float4(demowindow.sx - rin.textborder - controllerwindowwidth, rin.ystarts[5], demowindow.sx - rin.textborder, rin.ystarts[5] + controllerwindowwidth);
    
    CHECK(!pms.setting_axistype_demo.empty());
    {
      GfxWindow gfxw(demowindow, 1.0);
      pms.setting_axistype_demo->renderToScreen();
    }
    
    drawBottomBlock(rin, 2);
    setColor(C::inactive_text);
    drawJustifiedText("Press your \"accept\" button to continue the demo.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.ystarts.size() - 2]), TEXT_CENTER, TEXT_MIN);
    drawJustifiedText("Press your \"cancel\" button to abort the demo.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.ystarts.size() - 1]), TEXT_CENTER, TEXT_MIN);
    
    setColor(C::inactive_text);
    if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_cursegment == 0) {
      drawText("Move controller", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("forward and", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("back to drive", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("forward and", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
      drawText("back.", rin.textsize, Float2(rin.xstart, rin.ystarts[5]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_cursegment == 1) {
      drawText("Move controller", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("side to side", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("to turn.", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING && pms.setting_axistype_demo_cursegment == 2) {
      drawText("Combine these", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("to drive around.", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_ABSOLUTE && pms.setting_axistype_demo_cursegment == 0) {
      drawText("Move controller", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("towards where", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("you want the", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("tank to go.", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_ABSOLUTE && pms.setting_axistype_demo_cursegment == 1) {
      drawText("The computer", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("will try to", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("turn your tank", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("in that direction.", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_TANK && pms.setting_axistype_demo_cursegment == 0) {
      drawText("Control treads", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("independently.", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("Your left stick", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("moves your left", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
      drawText("tank tread.", rin.textsize, Float2(rin.xstart, rin.ystarts[5]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_TANK && pms.setting_axistype_demo_cursegment == 1) {
      drawText("Your right stick", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("moves your right", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("tank tread.", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_TANK && pms.setting_axistype_demo_cursegment == 2) {
      drawText("Move both sticks", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("forward to move", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("your tank", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("forward.", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_TANK && pms.setting_axistype_demo_cursegment == 3) {
      drawText("Experiment with", rin.textsize, Float2(rin.xstart, rin.ystarts[1]));
      drawText("tank mode for", rin.textsize, Float2(rin.xstart, rin.ystarts[2]));
      drawText("very precise", rin.textsize, Float2(rin.xstart, rin.ystarts[3]));
      drawText("tank control.", rin.textsize, Float2(rin.xstart, rin.ystarts[4]));
    } else {
      CHECK(0);
    }
    
    const float widgetsize = 0.005;
    Float2 cont = pms.setting_axistype_demo_ai->getControls();
    cont.x += 1;
    cont.y += 1;
    cont /= 2;
  
    setColor(C::gray(1.0));
    if(pms.setting_axistype_curchoice / 2 == KSAX_STEERING || pms.setting_axistype_curchoice / 2 == KSAX_ABSOLUTE) {
      drawRect(controllerwindow, 0.0001);
      const Float4 livecwind = Float4(controllerwindow.sx + widgetsize, controllerwindow.sy + widgetsize, controllerwindow.ex - widgetsize, controllerwindow.ey - widgetsize);
      drawShadedRect(boxAround(Float2((livecwind.ex - livecwind.sx) * cont.x + livecwind.sx, (livecwind.sy - livecwind.ey) * cont.y + livecwind.ey), widgetsize), 0.00001, widgetsize);
    } else if(pms.setting_axistype_curchoice / 2 == KSAX_TANK) {
      const float xshift = widgetsize * 5;
      const float ys = controllerwindow.sy + widgetsize;
      const float ye = controllerwindow.ey - widgetsize;
      drawRect(Float4(controllerwindow.ex - xshift, controllerwindow.sy, controllerwindow.ex - xshift + widgetsize, controllerwindow.ey), 0.0001);
      drawRect(Float4(controllerwindow.ex - widgetsize, controllerwindow.sy, controllerwindow.ex, controllerwindow.ey), 0.0001);
      drawShadedRect(boxAround(Float2(controllerwindow.ex - xshift + widgetsize / 2, (ys - ye) * cont.x + ye), widgetsize), 0.00001, widgetsize);
      drawShadedRect(boxAround(Float2(controllerwindow.ex - widgetsize / 2, (ys - ye) * cont.y + ye), widgetsize), 0.00001, widgetsize);
    } else {
      CHECK(0);
    }
    
  } else if(pms.settingmode == SETTING_AXISCHOOSE) {
    StandardButtonRenderData sbrd;
    sbrd.rin = &rin;
    sbrd.buttons = &pms.axes;
    sbrd.inverts = &pms.axes_invert;
    sbrd.names = &ksax_axis_names[pms.setting_axistype];
    sbrd.groupnames = NULL;
    sbrd.groups = axis_groups;
    sbrd.sel_button = pms.setting_axis_current;
    sbrd.sel_button_reading = pms.setting_axis_reading;
    sbrd.prefix = "Axis #";
    sbrd.description.push_back("Configure your control directions. Select the entry,");
    sbrd.description.push_back("then move your controller in the desired direction.");
    sbrd.description.push_back("Choose \"done\" when ready.");
    
    standardButtonRender(sbrd);
  } else if(pms.settingmode == SETTING_TEST) {
    drawBottomBlock(rin, 2);
    setColor(C::inactive_text);
    drawJustifiedText("Test your tank controls.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.textline_count - 2]), TEXT_CENTER, TEXT_MIN);
    if(pms.choicemode == CHOICE_IDLE) {
      drawJustifiedText("Push your \"accept\" button to enter the test.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    } else {
      drawJustifiedText("Push your \"cancel\" button once you're done.", rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    }
    GfxWindow gfxw(Float4(rin.xstart, rin.ystarts[2], rin.xend, rin.ystarts[rin.textline_count - 4]), 1.0);
    pms.test_game->renderToScreen();
  } else if(pms.settingmode == SETTING_READY) {
    setColor(C::inactive_text);
    const char * const text[] = {"You've finished setting up your controls. Push ", "\"accept\" if you're finished. Go back and edit your", "settings if you are dissatisfied. You may return to", "change settings any time the shop is available."};
    int stp = (rin.textline_count - 1 - sizeof(text) / sizeof(*text)) / 2;
    drawTextBoxAround(Float4(rin.xstart, rin.ystarts[stp], rin.xend, rin.ystarts[stp + sizeof(text) / sizeof(*text) - 1] + rin.textsize), rin.textsize);
    for(int i = 0; i < sizeof(text) / sizeof(*text); i++)
      drawJustifiedText(text[i], rin.textsize, Float2((rin.xstart + rin.xend) / 2, rin.ystarts[i + stp]), TEXT_CENTER, TEXT_MIN);
  } else {
    CHECK(0);
  }
  
  // Finally, we render our topic line and box, so it can fit around the rest without getting accidentally erased by our bottom-of-box-erase code.
  {
    vector<pair<float, float> > xpos = choiceTopicXpos(rin.external_xstart, rin.external_xend, rin.textsize);
    
    for(int dist = xpos.size(); dist >= 0; --dist) {
      for(int i = 0; i < xpos.size(); i++) {
        if(abs(i - pms.settingmode) != dist)
          continue;
        
        // First, opacity for the bottom chunk
        {
          vector<Float2> path;
          path.push_back(Float2(rin.drawzone.sx, rin.expansion_bottom));
          path.push_back(Float2(xpos[i].first - rin.border, rin.tab_bottom));
          path.push_back(Float2(xpos[i].second + rin.border, rin.tab_bottom));
          path.push_back(Float2(rin.drawzone.ex, rin.expansion_bottom));
          drawSolidLoop(path);
        }
        
        setColor(((i == pms.settingmode && pms.choicemode == CHOICE_IDLE) ? C::active_text : C::inactive_text));
        
        // Next, draw text
        drawText(setting_names[i], rin.textsize, Float2(xpos[i].first, rin.ystarts[0]));
        
        setColor(i == pms.settingmode ? C::active_text : C::inactive_text);
        
        // Now our actual path, left to right
        vector<Float2> path;
        path.push_back(Float2(rin.drawzone.sx, rin.expansion_bottom));
        path.push_back(Float2(xpos[i].first - rin.border, rin.tab_bottom));
        path.push_back(Float2(xpos[i].first - rin.border, rin.tab_top));
        path.push_back(Float2(xpos[i].second + rin.border, rin.tab_top));
        path.push_back(Float2(xpos[i].second + rin.border, rin.tab_bottom));
        path.push_back(Float2(rin.drawzone.ex, rin.expansion_bottom));
        drawLinePath(path, i == pms.settingmode ? rin.linethick : rin.linethick / 2);
      }
    }
    
    setColor(C::active_text);
    {
      vector<Float2> path;
      path.push_back(Float2(rin.drawzone.sx, rin.expansion_bottom));
      path.push_back(Float2(rin.drawzone.sx, rin.drawzone.ey));
      path.push_back(Float2(rin.drawzone.ex, rin.drawzone.ey));
      path.push_back(Float2(rin.drawzone.ex, rin.expansion_bottom));
      drawLinePath(path, rin.linethick);
    }
  }
}
