
#include "metagame_config.h"

#include "adler32_util.h"
#include "audio.h"
#include "game.h"
#include "game_projectile.h"
#include "game_tank.h"
#include "gfx.h"

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
  
  RenderInfo(float aspect) : aspect(aspect) {
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
  
  setting_axistype = KSAX_STEERING;
  
  reset_controls();
}

void PlayerMenuState::reset_controls() {
  setting_button_current = -1;
  
  fill(buttons.begin(), buttons.end(), -1);
  fill(axes.begin(), axes.end(), -1);
  fill(axes_invert.begin(), axes_invert.end(), false);

  buttons.resize(BUTTON_LAST, -1);
  axes.resize(2, -1);
  axes_invert.resize(axes.size(), false);
  
  axes[1] = 1;
}


PlayerMenuState::~PlayerMenuState() { }

Keystates PlayerMenuState::genKeystate(const Controller &keys) const {
  StackString sstr("genkeystate");
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  CHECK(SIMUL_WEAPONS == 4);
  
  {
    bool error = false;
    for(int i = 0; i < buttons.size(); i++) {
      if(buttons[i] < 0 || buttons[i] >= keys.keys.size()) {
        dprintf("Key range: button %d is %d (max %d)\n", i, buttons[i], keys.keys.size());
        error = true;
      }
    }
    for(int i = 0; i < axes.size(); i++) {
      if(axes[i] < 0 || axes[i] >= keys.axes.size()) {
        dprintf("Key range: axis %d is %d (max %d)\n", i, axes[i], keys.axes.size());
        error = true;
      }
    }
    CHECK(!error);
  }
  
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
  
  kst.accept_or_fire = kst.accept;
  for(int i = 0; i < ARRAY_SIZE(kst.fire); i++)
    kst.accept_or_fire |= kst.fire[i];
  
  return kst;
}

enum TriggerType { TT_BUTTON, TT_AXIS };

struct StandardButtonTickData {
  vector<int> *outkeys;
  vector<char> *outinvert;
  
  vector<TriggerType> desiredkeytype;
  
  int *current_button;
  
  int accept_button;

  Controller keys;
  vector<Coord> triggers;
  vector<Coord> oldtriggers;
  vector<TriggerType> triggertype;
  
  CannedKeys ck;
};

struct StandardButtonRenderData {
  const RenderInfo *rin;
  
  int sel_button;
  string sel_text;
  
  vector<string> description;
  
  CannedKeys ck;
};

bool standardButtonTick(StandardButtonTickData *sbtd) {
  StackString sstr("standardButtonTick");
  
  CHECK(sbtd);
  CHECK(sbtd->outkeys);
  CHECK(sbtd->outinvert);
  CHECK(sbtd->outkeys->size());
  CHECK(sbtd->outkeys->size() == sbtd->outinvert->size());
  CHECK(sbtd->outkeys->size() == sbtd->desiredkeytype.size());
  CHECK(sbtd->oldtriggers.size() == sbtd->triggers.size());
  CHECK(sbtd->oldtriggers.size() == sbtd->triggertype.size());
  
  if(*sbtd->current_button == -1) {
    if(sbtd->ck.canned) {
      dprintf("Initting canned\n");
      *sbtd->current_button = sbtd->outkeys->size();
      
      for(int i = 0; i < ARRAY_SIZE(button_order); i++) {
        (*sbtd->outkeys)[i] = sbtd->ck.button_layout[i];
        (*sbtd->outinvert)[i] = false;
      }
      dprintf("Canned done\n");
      return false; // We wait a cycle so we can have things like sbtd->accept_button filled in for us. Go lazy.
    } else {
      *sbtd->current_button = 0;
    }
  }
  CHECK(*sbtd->current_button >= 0 && *sbtd->current_button <= sbtd->outkeys->size());
  
  bool assigned = false;
  
  // First off, let's see if we do successfully change buttons.
  if(*sbtd->current_button < sbtd->outkeys->size()) {
    for(int i = 0; i < sbtd->triggers.size(); i++) { // For each input button . . .
      if(sbtd->desiredkeytype[*sbtd->current_button] == sbtd->triggertype[i] && abs(sbtd->triggers[i]) > 0.9 && abs(sbtd->oldtriggers[i]) <= 0.9) { // If button was pushed . . .
        bool valid = true;
        for(int j = 0; j < sbtd->outkeys->size(); j++) {
          if(sbtd->desiredkeytype[j] == sbtd->desiredkeytype[*sbtd->current_button] && (*sbtd->outkeys)[j] == i) {
            valid = false;
            break;
          }
        }
        if(valid) {
          queueSound(S::choose);
          (*sbtd->outkeys)[*sbtd->current_button] = i;
          (*sbtd->outinvert)[*sbtd->current_button] = (sbtd->triggers[i] < 0);
          (*sbtd->current_button)++;
          assigned = true;
          break;
        } else {
          queueSound(S::error);
        }
      }
    }
  }

  // Here's where we potentially quit.
  if(*sbtd->current_button == sbtd->outkeys->size() && !assigned) {
    if(sbtd->keys.keys[sbtd->accept_button].push) {
      queueSound(S::accept);
      *sbtd->current_button = -1;
      return true;  // then we're done.
    }
    
    bool anythingpushed = false;
    for(int i = 0; i < sbtd->keys.keys.size(); i++)
      if(sbtd->keys.keys[i].push)
        anythingpushed = true;
    if(anythingpushed) {
      queueSound(S::accept);
      sbtd->outkeys->clear();
      sbtd->outkeys->resize(sbtd->outinvert->size(), -1);
      *sbtd->current_button = -1;
      // ABORT ABORT ABORT
    }
  }
  
  return false;
}

void drawBottomBlock(const RenderInfo &rin, int lines) {
  float bottom_point = rin.ystarts[rin.textline_count - lines] - getTextBoxBorder(rin.textsize);
  drawSolid(Float4(rin.drawzone.sx, bottom_point, rin.drawzone.ex, rin.drawzone.ey));
  setColor(C::box_border);
  drawLine(Float4(rin.drawzone.sx, bottom_point, rin.drawzone.ex, bottom_point), getTextBoxThickness(rin.textsize));
}

void drawButton(const Float2 &pos, char kar) {
  const float bevel = 0.1;
  const float border = 0.2;
  
  const float highv = 1.0 - border;
  
  setColor(C::inactive_text);
  
  vector<Float2> path;
  path.push_back(pos + Float2(bevel, 0));
  path.push_back(pos + Float2(0, bevel));
  path.push_back(pos + Float2(0, highv - bevel));
  path.push_back(pos + Float2(bevel, highv));
  path.push_back(pos + Float2(highv - bevel, highv));
  path.push_back(pos + Float2(highv, highv - bevel));
  path.push_back(pos + Float2(highv, bevel));
  path.push_back(pos + Float2(highv - bevel, 0));
  
  drawLineLoop(path, 0.05);
  
  setColor(C::active_text);
  
  drawJustifiedText(StringPrintf("%c", kar), 0.4, pos + Float2(highv / 2, highv / 2), TEXT_CENTER, TEXT_CENTER);
}

static bool controller_loaded = false;
static Dvec2 controller_front;
static Dvec2 controller_top;

void standardButtonRender(const StandardButtonRenderData &sbrd) {
  StackString sstr("standardButtonRender");
  CHECK(sbrd.rin);
  
  if(!controller_loaded) {
    controller_front = loadDvec2(FLAGS_fileroot + "controller_front.dv2");
    controller_top = loadDvec2(FLAGS_fileroot + "controller_top.dv2");
    controller_loaded = true;
  }
  
  int cb = sbrd.sel_button;
  string text = sbrd.sel_text;
  if(cb == ARRAY_SIZE(button_order) || sbrd.ck.canned) {
    cb = 0;
    text = "Push \"accept\" to continue, or any other key to restart";
  } else if(cb == -1) {
    cb = 0;
  }
  
  if(sbrd.ck.canned) {
    /*
    setColor(Color(C::active_text));
    Float2 origin = Float2((sbrd.rin->xstart + sbrd.rin->xend) / 2, (sbrd.rin->ystarts[1] + sbrd.rin->ystarts[sbrd.rin->ystarts.size() - 2]) / 2);
    float mw = 0;
    for(int i = 0; i < sbrd.ck.descriptive_text.size(); i++)
      mw = max(mw, getTextWidth(sbrd.ck.descriptive_text[i], sbrd.rin->textsize));
    float mh = getTextHeight(sbrd.ck.descriptive_text.size(), sbrd.rin->textsize);
    
    drawTextBoxAround(Float4(origin.x - mw / 2, origin.y - mh / 2, origin.x + mw / 2, origin.y + mh / 2), sbrd.rin->textsize);
    drawJustifiedMultiText(sbrd.ck.descriptive_text, sbrd.rin->textsize, origin, TEXT_CENTER, TEXT_CENTER);
    text = "Push \"accept\" to continue";*/
    
    Float4 bounds = Float4(sbrd.rin->xstart + sbrd.rin->textsize, sbrd.rin->ystarts[2], sbrd.rin->xend - sbrd.rin->textsize, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - 2]);
    drawTextBoxAround(bounds, sbrd.rin->textsize);
    
    GfxWindow gfxw(bounds, 1.0);
    
    if(!sbrd.ck.is_second) {
      setZoomAround(Float4(-0.5, 0, 10, 6));
      
      drawButton(Float2(0, 1), '7');
      drawButton(Float2(1, 1), '8');
      drawButton(Float2(2, 1), '9');
      drawButton(Float2(3, 1), '0');
      drawJustifiedText("Weapons 1 through 4", 0.4, Float2(2, 0.5), TEXT_CENTER, TEXT_CENTER);
      
      drawButton(Float2(2.5, 2.5), 'O');
      drawButton(Float2(3.5, 2.5), 'P');
      drawJustifiedText("Accept", 0.4, Float2(2.2, 3), TEXT_MAX, TEXT_CENTER);
      drawJustifiedText("Cancel", 0.4, Float2(4.7, 3), TEXT_MIN, TEXT_CENTER);
      
      drawButton(Float2(4.5, 5), '/');
      drawJustifiedText("Precision", 0.4, Float2(5, 4.5), TEXT_CENTER, TEXT_CENTER);
      
      drawButton(Float2(7, 4), '<');
      drawButton(Float2(8, 4), 'v');
      drawButton(Float2(9, 4), '>');
      drawButton(Float2(8, 3), '^');
      drawJustifiedText("Movement", 0.4, Float2(8.5, 2.5), TEXT_CENTER, TEXT_CENTER);
    } else {
      setZoomAround(Float4(-1.0, 0, 9.5, 6));
      
      drawButton(Float2(4, 3.5), 'C');
      drawButton(Float2(5, 3.5), 'V');
      drawButton(Float2(6, 3.5), 'B');
      drawButton(Float2(7, 3.5), 'N');
      drawJustifiedText("Weapons 1 through 4", 0.4, Float2(6, 5), TEXT_CENTER, TEXT_CENTER);
      
      drawButton(Float2(4.5, 2), 'F');
      drawButton(Float2(6, 2), 'G');
      drawJustifiedText("Accept", 0.4, Float2(4.5, 1.5), TEXT_CENTER, TEXT_CENTER);
      drawJustifiedText("Cancel", 0.4, Float2(7, 1.5), TEXT_CENTER, TEXT_CENTER);
      
      drawButton(Float2(1.5, 3.5), 'X');
      drawJustifiedText("Precision", 0.4, Float2(1.2, 4), TEXT_MAX, TEXT_CENTER);
      
      drawButton(Float2(0, 2), 'A');
      drawButton(Float2(1, 2), 'S');
      drawButton(Float2(2, 2), 'D');
      drawButton(Float2(0.8, 1), 'W');
      drawJustifiedText("Movement", 0.4, Float2(1.5, 0.5), TEXT_CENTER, TEXT_CENTER);
    }
  } else {
    const int realite = button_order[cb];
    Dvec2 renderobj;
    if(frameNumber / (FPS / 3) % 3 == 0) {
      renderobj = loadDvec2(StringPrintf("%scontroller_push%d.dv2", FLAGS_fileroot.c_str(), cb + 1));
    } else {
      if(realite == BUTTON_FIRE1 || realite == BUTTON_FIRE2 || realite == BUTTON_FIRE3 || realite == BUTTON_FIRE4) {
        renderobj = controller_top;
      } else {
        renderobj = controller_front;
      }
    }
    
    Float4 controllersize = Float4(sbrd.rin->xstart, sbrd.rin->ystarts[1], sbrd.rin->xend, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - 2]);
    
    {
      GfxInvertingStencil gfxis;
      stencilVectorPath(renderobj.paths[0], controllersize, 20);
    }
    
    {
      GfxStenciled gfxs;
      drawSolid(controllersize);
    }
    
    setColor(Color(0.85, 1.0, 0.8));
    drawDvec2(renderobj, Float4(sbrd.rin->xstart, sbrd.rin->ystarts[1], sbrd.rin->xend, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - 2]), 20, sbrd.rin->linethick * 2);
  }
  
  setColor(Color(C::active_text));
  drawJustifiedText(text, sbrd.rin->textsize, Float2((sbrd.rin->xstart + sbrd.rin->xend) / 2, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - 1]), TEXT_CENTER, TEXT_MIN);
  
  /*drawBottomBlock(*sbrd.rin, sbrd.description.size());
  setColor(C::inactive_text);
  for(int i = 0; i < sbrd.description.size(); i++)
    drawJustifiedText(sbrd.description[i], sbrd.rin->textsize, Float2((sbrd.rin->xstart + sbrd.rin->xend) / 2, sbrd.rin->ystarts[sbrd.rin->ystarts.size() - sbrd.description.size() + i]), TEXT_CENTER, TEXT_MIN);*/
  
}

vector<pair<float, float> > choiceTopicXpos(float sx, float ex, float textsize, int maxi) {
  vector<pair<float, float> > rv;
  float jtx = 0;
  for(int i = 0; i < maxi; i++) {
    
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

bool runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions, const ControlConsts &cc) {
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
    
    bool keyed = false;
    for(int j = 0; j < keys.keys.size(); j++)
        if(keys.keys[j].push)
          keyed = true;
    
    if(keyed) {
      if(targetInside != -1) {            
        pms->faction = &factions[targetInside];
        pms->settingmode = SETTING_BUTTONS;
        pms->choicemode = CHOICE_FIRSTPASS; // Because it is.
        factions[targetInside].taken = true;
        queueSound(S::accept);
      } else {
        dprintf("RESET\n");
        *pms = PlayerMenuState(); // RESET
        return true;
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
    
    // Here's how we want this to be structured.
    // If we're idle, and in buttons, and we press any button, it should clear the keymap, use that button as a command, and switch to active.
    // If we're idle and elsewhere, the old behavior persists.
    
    if(pms->choicemode == CHOICE_IDLE && pms->settingmode == SETTING_BUTTONS && !cc.ck.canned) {
      bool pushedbutton = false;
      for(int i = 0; i < keys.keys.size(); i++)
        if(keys.keys[i].push)
          pushedbutton = true;
      
      if(pushedbutton) {
        pms->choicemode = CHOICE_FIRSTPASS;
        pms->reset_controls();
      }
      
      // Now the button will be echoed through.
    }
    
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
        queueSound(S::choose);
        pms->choicemode = CHOICE_ACTIVE;
      }
    } else if(pms->settingmode == SETTING_BUTTONS) {
      StackString sstr("butans");
      
      vector<Coord> triggers;
      vector<Coord> oldtriggers;
      
      vector<int> buttons;
      vector<char> invert;
      
      vector<TriggerType> triggertype;
      
      for(int i = 0; i < keys.keys.size(); i++) {
        triggers.push_back(keys.keys[i].down);
        if(keys.keys[i].down)
          oldtriggers.push_back(!keys.keys[i].push);
        else
          oldtriggers.push_back(!keys.keys[i].release);
        triggertype.push_back(TT_BUTTON);
      }
      
      for(int i = 0; i < keys.axes.size(); i++) {
        triggers.push_back(keys.axes[i]);
        oldtriggers.push_back(keys.lastaxes[i]);

        triggertype.push_back(TT_AXIS);
      }
      
      vector<TriggerType> desiredkeytype;
      for(int i = 0; i < ARRAY_SIZE(button_order); i++) {
        if(button_order[i] >= 0) {
          buttons.push_back(pms->buttons[button_order[i]]);
          invert.push_back(false);
          desiredkeytype.push_back(TT_BUTTON);
        } else {
          buttons.push_back(pms->axes[0] + keys.keys.size());
          invert.push_back(pms->axes_invert[0]);
          desiredkeytype.push_back(TT_AXIS);
        }
      }
      
      StandardButtonTickData sbtd;
      sbtd.outkeys = &buttons;
      sbtd.outinvert = &invert;
      sbtd.desiredkeytype = desiredkeytype;
      sbtd.current_button = &pms->setting_button_current;
      sbtd.accept_button = pms->buttons[BUTTON_ACCEPT];
      sbtd.keys = keys;
      sbtd.triggers = triggers;
      sbtd.oldtriggers = oldtriggers;
      sbtd.triggertype = triggertype;
      sbtd.ck = cc.ck;
      
      bool rv = standardButtonTick(&sbtd);
      
      for(int i = 0; i < ARRAY_SIZE(button_order); i++) {
        if(button_order[i] >= 0) {
          pms->buttons[button_order[i]] = buttons[i];
          CHECK(!invert[i]);
        } else {
          pms->axes[0] = buttons[i] - keys.keys.size();  // sigh
          pms->axes_invert[0] = invert[i];
        }
      }
      
      if(rv) {
        if(pms->choicemode == CHOICE_ACTIVE) {
          pms->choicemode = CHOICE_IDLE;
        } else if(pms->choicemode == CHOICE_FIRSTPASS && cc.ck.canned) {
          return true;
        } else if(pms->choicemode == CHOICE_FIRSTPASS && !cc.ck.canned) {
          pms->settingmode++;
        } else {
          CHECK(0);
        }
      }
      
      pms->setting_axistype = cc.mode;
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
      
      Float4 boundy = Float4(0, 0, 250, 100);
      boundy -= boundy.midpoint();
      
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
  const RenderInfo rin(getAspect());
  
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
    StackString sstr("buttons");
    vector<vector<string> > names;
    for(int i = 0; i < BUTTON_LAST; i++) {
      vector<string> tix;
      tix.push_back(cc.buttonnames[i]);
      names.push_back(tix);
    }
    
    string key_descr;
    {
      int rbc = pms.setting_button_current;
      if(rbc == -1)
        rbc = 0;
      if(rbc == ARRAY_SIZE(button_order))
        rbc = ARRAY_SIZE(button_order) - 1;
      CHECK(rbc >= 0 && rbc < ARRAY_SIZE(button_order));
      
      if(button_order[rbc] < 0)
        key_descr = "Move the right stick to the right.";
      else
        key_descr = cc.buttonnames[button_order[rbc]];
    }
    
    StandardButtonRenderData sbrd;
    sbrd.rin = &rin;
    sbrd.sel_button = pms.setting_button_current;
    sbrd.sel_text = key_descr;
    sbrd.description.push_back("Press the indicated buttons to configure your controller.");
    sbrd.ck = cc.ck;
    //if(!cc.availdescr.empty())
      //sbrd.description.push_back(cc.availdescr);
    
    standardButtonRender(sbrd);
  } else if(pms.settingmode == SETTING_TEST) {
    StackString sstr("test");
    drawBottomBlock(rin, 2);
    setColor(C::inactive_text);
    drawJustifiedText("Test your tank controls.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 2]), TEXT_CENTER, TEXT_MIN);
    if(pms.choicemode == CHOICE_IDLE) {
      drawJustifiedText("Push your \"accept\" button to enter the test.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    } else {
      drawJustifiedText("Push your \"cancel\" button once you're done.", rin.textsize, Float2(rin.xcenter, rin.ystarts[rin.textline_count - 1]), TEXT_CENTER, TEXT_MIN);
    }
    float height = rin.ystarts[rin.textline_count - 4] - rin.ystarts[2];
    float twid = min(height * 2.5f, rin.xend - rin.xstart);
    float xmid = (rin.xstart + rin.xend) / 2;
    GfxWindow gfxw(Float4(xmid - twid / 2, rin.ystarts[2], xmid + twid / 2, rin.ystarts[rin.textline_count - 4]), 1.0);
    pms.test_game->renderToScreen();
  } else if(pms.settingmode == SETTING_READY) {
    StackString sstr("ready");
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
    StackString sstr("topicline");
    int maxi = SETTING_LAST;
    if(cc.ck.canned)
      maxi = 1;
    
    vector<pair<float, float> > xpos = choiceTopicXpos(rin.external_xstart, rin.external_xend, rin.textsize, maxi);
    
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
  if(pms.faction)
    adler(adl, *pms.faction);
  else
    adl->addByte(0);
  adler(adl, pms.settingmode);
  adler(adl, pms.choicemode);
  adler(adl, pms.buttons);
  adler(adl, pms.axes);
  adler(adl, pms.axes_invert);
  adler(adl, pms.setting_axistype);
}
