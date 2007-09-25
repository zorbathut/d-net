
#include "metagame_config.h"

#include "audio.h"
#include "debug.h"
#include "game.h"
#include "game_tank.h"
#include "gfx.h"
#include "player.h"
#include "adler32_util.h"

using namespace std;

class RenderInfo {
public:
  // Basic math!
  static const float textline_size;  // How many times larger a line of text is than its divider
  static const float border_size;
  static const float top_border_extra_size;
  static const float divider_size;  // Division between tabs and content
  static const float units;

  static const int textline_count = 17; // How many lines we're going to have
  static const int demo_line_end = 6;
  static const float linethick;

  // runtime constants
  Float4 drawzone;
  float unitsize;
  float border;
  float external_xstart;
  float external_xend;
  float xstart;
  float xend;
  float xcenter;
  vector<float> ystarts;

  float textsize;
  float textborder;

  float tab_top;
  float tab_bottom;
  float expansion_bottom;

  float aspect;
  
  RenderInfo() {
    aspect = 1.532563;  // lolz
    
    const float roundaborder = 0.05;
    
    drawzone = Float4(roundaborder, roundaborder, aspect - roundaborder, 1.0 - roundaborder);     // maaaagic

    // runtime constants
    unitsize = drawzone.span_y() / units;
    border = unitsize * border_size;
    external_xstart = drawzone.sx + border;
    external_xend = drawzone.ex - border;
    ystarts.resize(textline_count);
    ystarts[0] = drawzone.sy + unitsize * top_border_extra_size;
    textsize = unitsize * textline_size;
    xstart = external_xstart + textsize * 2;
    xend = external_xend - textsize * 2;
    xcenter = (xstart + xend) / 2;
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
const float RenderInfo::divider_size = 5;  // Division between tabs and content in dividers
const float RenderInfo::units = textline_size * textline_count + textline_count - 1 + border_size + top_border_extra_size + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders, plus the increased divider from categories to data
const float RenderInfo::linethick = 0.003;

PlayerMenuState::PlayerMenuState() {
  faction = NULL;
  compasspos = Coord2(0, 0);
  
  current_faction_over = -1;
  current_faction_over_duration = 0;

  settingmode = SETTING_BUTTONS;
  choicemode = CHOICE_FIRSTPASS;
  
  setting_button_current = -1;
  setting_axis_current = -1;
  
  setting_axistype = KSAX_STEERING;
  
  buttons.resize(BUTTON_LAST, -1);
  axes.resize(2, -1);
  axes_invert.resize(axes.size(), false);
}

PlayerMenuState::~PlayerMenuState() { }

Keystates PlayerMenuState::genKeystate(const Controller &keys) const {
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  CHECK(SIMUL_WEAPONS == 4);
  kst.accept = keys.keys[buttons[BUTTON_ACCEPT]];
  kst.cancel = keys.keys[buttons[BUTTON_CANCEL]];
  kst.precision = keys.keys[buttons[BUTTON_PRECISION]];
  kst.fire[0] = keys.keys[buttons[BUTTON_FIRE1]];
  kst.fire[1] = keys.keys[buttons[BUTTON_FIRE2]];
  kst.fire[2] = keys.keys[buttons[BUTTON_FIRE3]];
  kst.fire[3] = keys.keys[buttons[BUTTON_FIRE4]];
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
  
  vector<int> groups;
  
  Controller keys;
  const vector<Coord> *triggers;
  const vector<Coord> *oldtriggers;
};

struct StandardButtonRenderData {
  const RenderInfo *rin;
  
  const vector<int> *buttons;
  const vector<char> *inverts;
  const vector<vector<string> > *names;
  
  const vector<string> *groupnames;
  vector<int> groups;
  
  int sel_button;
  ReadMode sel_button_reading;
  
  string prefix;
  vector<string> description;
};

bool changeButtons(vector<int> *buttons, vector<char> *inversions, const vector<int> &groups, int choice, int button, bool inverted) {
  // Step 1: If this button has been chosen somewhere else, and we're not choosing this button for the first time, swap 'em (TODO: go back?)
  for(int j = 0; j < buttons->size(); j++) {
    //if(groups[j] != groups[choice])
      //continue;
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
  CHECK(sbtd);
  CHECK(sbtd->outkeys);
  CHECK(sbtd->outkeys->size());
  CHECK(sbtd->triggers);
  CHECK(!sbtd->oldtriggers || sbtd->oldtriggers->size() == sbtd->triggers->size());
  if(sbtd->outinvert)
    CHECK(sbtd->outkeys->size() == sbtd->outinvert->size());
  if(*sbtd->current_button == -1) {
    if(sbtd->require_trigger) {
      if((*sbtd->outkeys)[0] == -1) {
        *sbtd->current_mode = RM_CHOOSING;
      } else {
        *sbtd->current_mode = RM_IDLE;
      }
    } else {
      *sbtd->current_mode = RM_NOTRIGGER;
    }
    *sbtd->current_button = 0;
  }
  CHECK(*sbtd->current_button >= 0 && *sbtd->current_button <= sbtd->outkeys->size());
  
  CHECK((*sbtd->current_mode == RM_NOTRIGGER) == !sbtd->require_trigger);
  CHECK(sbtd->accept_button != -1 && sbtd->cancel_button != -1 || !sbtd->require_trigger);
  
  bool nomove = false;
  
  // First off, let's see if we do successfully change buttons.
  if((*sbtd->current_mode == RM_NOTRIGGER || *sbtd->current_mode == RM_CHOOSING) && *sbtd->current_button < sbtd->outkeys->size()) {
    for(int i = 0; i < sbtd->triggers->size(); i++) { // For each input button . . .
      if(abs((*sbtd->triggers)[i]) > 0.9 && (!sbtd->oldtriggers || abs((*sbtd->oldtriggers)[i]) <= 0.9)) { // If button was pushed . . .
        int oldbutton = (*sbtd->outkeys)[*sbtd->current_button];
        if(changeButtons(sbtd->outkeys, sbtd->outinvert, sbtd->groups, *sbtd->current_button, i, (*sbtd->triggers)[i] < 0)) { // If button successfully changed . . .
          queueSound(S::choose);
          if(oldbutton == -1) {
            (*sbtd->current_button)++;
            if(*sbtd->current_button == sbtd->outkeys->size() && *sbtd->current_mode == RM_CHOOSING) {
              *sbtd->current_mode = RM_IDLE;
              nomove = true;
            }
          } else if(*sbtd->current_mode == RM_NOTRIGGER) {
          } else if(*sbtd->current_mode == RM_CHOOSING) {
            *sbtd->current_mode = RM_IDLE;
          } else {
            CHECK(0);
          }
          break;
        } else {
          queueSound(S::error);
        }
      }
    }
  }
  
  // Now let's see if we move.
  if(!nomove && (*sbtd->current_mode == RM_NOTRIGGER || *sbtd->current_mode == RM_IDLE)) {
    if(sbtd->keys.u.repeat) {
      queueSound(S::select);
      (*sbtd->current_button)--;
    }
    if(sbtd->keys.d.repeat) {
      queueSound(S::select);
      (*sbtd->current_button)++;
    }
    *sbtd->current_button = modurot(*sbtd->current_button, sbtd->outkeys->size() + 1);
    
    // Here's where we potentially quit.
    if(*sbtd->current_button == sbtd->outkeys->size() && sbtd->accept_button != -1 && sbtd->keys.keys[sbtd->accept_button].push /*|| sbtd->cancel_button != -1 && sbtd->keys.keys[sbtd->cancel_button].push*/) { // if we're on done AND the accept button exists AND the accept button was pushed OR the cancel button exists AND the cancel button was pushed . . .
      if(count(sbtd->outkeys->begin(), sbtd->outkeys->end(), -1) == 0) { // AND there are no unfinished keys . . .
        *sbtd->current_button = -1;
        queueSound(S::accept);
        return true;  // then we're done.
      } else {
        queueSound(S::error);
      }
    }
  }
  
  // Now let's see if we enter CHOOSING state. Only if we're not on DONE.
  if(*sbtd->current_mode == RM_IDLE && *sbtd->current_button != sbtd->outkeys->size() && sbtd->keys.keys[sbtd->accept_button].push) {
    queueSound(S::accept);
    (*sbtd->current_mode) = RM_CHOOSING;
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
  CHECK(sbrd.buttons->size() == sbrd.names->size());
  CHECK(sbrd.buttons->size() == sbrd.groups.size());
  CHECK(!sbrd.inverts || sbrd.buttons->size() == sbrd.inverts->size());
  
  int linesneeded = 2;
  for(int i = 0; i < sbrd.names->size(); i++)
    linesneeded += (*sbrd.names)[i].size();
  
  {
    set<int> grid(sbrd.groups.begin(), sbrd.groups.end());
    CHECK(grid.size() > 0);
    linesneeded += grid.size() - 1;
    
    vector<int> grod = sbrd.groups;
    grod.erase(unique(grod.begin(), grod.end()), grod.end());
    CHECK(grod.size() == grid.size());
    
    CHECK(!sbrd.groupnames || grid.size() == sbrd.groupnames->size());
    for(int i = 0; i < grid.size(); i++)
      CHECK(grid.count(i));
    
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
    if(sbrd.sel_button == i) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    CHECK(i < sbrd.names->size());
    CHECK(cy < sbrd.rin->ystarts.size());
    for(int j = 0; j < (*sbrd.names)[i].size(); j++)
      drawText((*sbrd.names)[i][j], sbrd.rin->textsize, Float2(xps, sbrd.rin->ystarts[cy++]));
    string btext;
    if(sbrd.sel_button == i && ((*sbrd.buttons)[i] == -1 || sbrd.sel_button_reading == RM_CHOOSING)) {
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
  
  if(sbrd.inverts)
    drawText("Done", sbrd.rin->textsize, Float2(groupnamexps, sbrd.rin->ystarts[cy++]));
  else
    drawText("Done (use accept button)", sbrd.rin->textsize, Float2(groupnamexps, sbrd.rin->ystarts[cy++]));  // shut up
  CHECK(cy == scy + linesneeded);

  drawBottomBlock(*sbrd.rin, sbrd.description.size());
  setColor(C::inactive_text);
  for(int i = 0; i < sbrd.description.size(); i++)
    drawJustifiedText(sbrd.description[i], sbrd.rin->textsize, Float2((sbrd.rin->xstart + sbrd.rin->xend) / 2, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - sbrd.description.size() + i]), TEXT_CENTER, TEXT_MIN);
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

bool runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  StackString sstr("runSettingTick");
  if(!pms->faction) { // if player hasn't chosen faction yet
    StackString sstr("chfact");
    {
      Coord2 dir = deadzone(keys.menu, DEADZONE_CENTER, 0.2) * 0.02;
      dir.y *= -1;
      pms->compasspos += dir;
      
      if(len(keys.menu) > 0.5)
        pms->current_faction_over_duration = 0;
    }
    {
      Coord4 bbx = startCBoundBox();
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
          queueSound(S::accept);
        }
      }
    }
    
    if(pms->current_faction_over == targetInside) {
      pms->current_faction_over_duration++;
    } else {
      pms->current_faction_over = targetInside;
      pms->current_faction_over_duration = 0;
      queueSound(S::cursorover);
    }
  } else {
    StackString sstr("proc");
    
    if(pms->choicemode == CHOICE_IDLE) {
      CHECK(pms->faction);
      if(keys.l.push) {
        queueSound(S::select);
        pms->settingmode--;
      }
      if(keys.r.push) {
        pms->settingmode++;
        queueSound(S::select);
      }
      if(pms->settingmode < 0)
        pms->settingmode = 0;
      if(pms->settingmode >= SETTING_LAST)
        pms->settingmode = SETTING_LAST - 1;
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push && pms->settingmode == SETTING_READY) {
        queueSound(S::accept);
        return true;
      }
      if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
        dprintf("pushactive\n");
        queueSound(S::choose);
        pms->choicemode = CHOICE_ACTIVE;
      }
    } else if(pms->settingmode == SETTING_BUTTONS) {
      vector<Coord> triggers;
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
      sbtd.groups = VECTORIZE(button_groups);
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      sbtd.oldtriggers = NULL;
      
      if(standardButtonTick(&sbtd)) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          pms->choicemode = CHOICE_IDLE;
        } else if(pms->choicemode == CHOICE_FIRSTPASS) {
          pms->settingmode++;
        } else {
          CHECK(0);
        }
      }
    } else if(pms->settingmode == SETTING_AXISCHOOSE) {
      if(keys.keys[pms->buttons[BUTTON_FIRE1]].down && keys.keys[pms->buttons[BUTTON_FIRE2]].down && keys.keys[pms->buttons[BUTTON_FIRE3]].down && keys.keys[pms->buttons[BUTTON_FIRE4]].down && (keys.keys[pms->buttons[BUTTON_FIRE1]].push || keys.keys[pms->buttons[BUTTON_FIRE2]].push || keys.keys[pms->buttons[BUTTON_FIRE3]].push || keys.keys[pms->buttons[BUTTON_FIRE4]].push))
        pms->setting_axistype = modurot(pms->setting_axistype + 1, KSAX_LAST);
        
      StackString sstr("SAX");
      
      StandardButtonTickData sbtd;      
      sbtd.outkeys = &pms->axes;
      sbtd.outinvert = &pms->axes_invert;
      sbtd.current_button = &pms->setting_axis_current;
      sbtd.current_mode = &pms->setting_axis_reading;
      sbtd.require_trigger = true;
      sbtd.accept_button = pms->buttons[BUTTON_ACCEPT];
      sbtd.cancel_button = pms->buttons[BUTTON_CANCEL];
      sbtd.groups = VECTORIZE(axis_groups);
      sbtd.keys = keys;
      sbtd.triggers = &keys.axes;
      sbtd.oldtriggers = &keys.lastaxes;
      
      if(standardButtonTick(&sbtd)) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          pms->choicemode = CHOICE_IDLE;
        } else if(pms->choicemode == CHOICE_FIRSTPASS) {
          pms->settingmode++;
        } else {
          CHECK(0);
        }
      }
    } else if(pms->settingmode == SETTING_TEST) {
      if(keys.keys[pms->buttons[BUTTON_CANCEL]].push) {
        queueSound(S::accept);
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
      
      pms->test_game->players.push_back(Player(pms->faction->faction, 0, Money(0)));
      
      const RenderInfo rin;
      
      Float4 boundy = Float4(rin.xstart, rin.ystarts[2], rin.xend, rin.ystarts[rin.textline_count - 4]);
      boundy -= boundy.midpoint();
      
      float mn = min(boundy.span_x(), boundy.span_y());
      boundy *= (100 / mn);
      
      pms->test_game->game.initTest(&pms->test_game->players[0], boundy);
    }
    
    // so is this
    if(pms->settingmode == SETTING_TEST) {
      StackString sstr("runtest");
      if(pms->choicemode == CHOICE_IDLE) {
        vector<Keystates> kst(1);
        CHECK(!pms->test_game->runTick(kst, &unsync()));
      } else if(pms->choicemode == CHOICE_ACTIVE || pms->choicemode == CHOICE_FIRSTPASS) {
        vector<Keystates> kst;
        kst.push_back(pms->genKeystate(keys));
        CHECK(!pms->test_game->runTick(kst, &unsync()));
      } else {
        CHECK(0);
      }
    }
  }
  
  return false;
}

void runSettingRender(const PlayerMenuState &pms, const ControlConsts &cc) {
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
      tix.push_back(cc.buttonnames[i]);
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
    sbrd.groups = VECTORIZE(button_groups);
    sbrd.sel_button = pms.setting_button_current;
    sbrd.sel_button_reading = pms.setting_button_reading;
    sbrd.prefix = "Button #";
    sbrd.description.push_back("Select your button setup. Choose \"done\" when ready.");
    if(!cc.availdescr.empty())
      sbrd.description.push_back(cc.availdescr);
    
    standardButtonRender(sbrd);
    
  } else if(pms.settingmode == SETTING_AXISCHOOSE) {
    StandardButtonRenderData sbrd;
    sbrd.rin = &rin;
    sbrd.buttons = &pms.axes;
    sbrd.inverts = &pms.axes_invert;
    sbrd.names = &ksax_axis_names[pms.setting_axistype];
    sbrd.groupnames = NULL;
    sbrd.groups = VECTORIZE(axis_groups);
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
    drawJustifiedText("Test your tank controls.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 2]), TEXT_CENTER, TEXT_MIN);
    if(pms.choicemode == CHOICE_IDLE) {
      drawJustifiedText("Push your \"accept\" button to enter the test.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    } else {
      drawJustifiedText("Push your \"cancel\" button once you're done.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    }
    GfxWindow gfxw(Float4(rin.xstart, rin.ystarts[2], rin.xend, rin.ystarts[rin.textline_count - 4]), 1.0);
    pms.test_game->renderToScreen();
  } else if(pms.settingmode == SETTING_READY) {
    setColor(C::inactive_text);
    const char * const text[] = {"You've finished setting up your controls. Push ", "\"accept\" if you're finished. Move your controller", "left and right to edit your settings if you", "are dissatisfied. You may return to change", "settings any time the shop is available."};
    int stp = (rin.textline_count - 1 - ARRAY_SIZE(text)) / 2;
    drawTextBoxAround(Float4(rin.xstart, rin.ystarts[stp], rin.xend, rin.ystarts[stp + ARRAY_SIZE(text) - 1] + rin.textsize), rin.textsize);
    for(int i = 0; i < ARRAY_SIZE(text); i++)
      drawJustifiedText(text[i], rin.textsize, Float2(rin.xcenter, rin.ystarts[i + stp]), TEXT_CENTER, TEXT_MIN);
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
          path.push_back(Float2(rin.drawzone.sx, rin.expansion_bottom + 0.01));
          path.push_back(Float2(rin.drawzone.sx, rin.expansion_bottom));
          path.push_back(Float2(xpos[i].first - rin.border, rin.tab_bottom));
          path.push_back(Float2(xpos[i].second + rin.border, rin.tab_bottom));
          path.push_back(Float2(rin.drawzone.ex, rin.expansion_bottom));
          path.push_back(Float2(rin.drawzone.ex, rin.expansion_bottom + 0.01));
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

void adler(Adler32 *adl, const FactionState &pms) {
  adler(adl, pms.taken);
  adler(adl, pms.compass_location);
  adler(adl, pms.faction);
}
void adler(Adler32 *adl, const PlayerMenuState &pms) {
  adler(adl, pms.compasspos);
  adler(adl, pms.faction);
  adler(adl, pms.settingmode);
  adler(adl, pms.choicemode);
  adler(adl, pms.buttons);
  adler(adl, pms.axes);
  adler(adl, pms.axes_invert);
  adler(adl, pms.setting_axistype);
}
