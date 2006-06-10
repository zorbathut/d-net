
#include "metagame_config.h"

#include "game.h"
#include "gfx.h"
#include "player.h"

using namespace std;

PlayerMenuState::PlayerMenuState() {
  settingmode = -12345;
  choicemode = -12345;

  setting_button_current = -1123;
  setting_button_reading = false;
  
  setting_axis_current = -1123;
  setting_axis_reading = false;
  
  setting_axistype = 1237539;
  
  test_game = NULL;
  test_player = NULL;
  
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
  
  setting_axistype = 0;
  
  test_game = NULL;
  test_player = NULL;
  
  buttons.resize(BUTTON_LAST, -1);
  axes.resize(2, -1);
  axes_invert.resize(axes.size(), false);
  
  faction = NULL;
  compasspos = cent;
}

bool PlayerMenuState::readyToPlay() const {
  return faction && settingmode == SETTING_READY && fireHeld == 60;
}

void PlayerMenuState::traverse_axistype(int delta, int axes) {
  do {
    setting_axistype += delta;
    setting_axistype += KSAX_END;
    setting_axistype %= KSAX_END;
  } while(ksax_minaxis[setting_axistype] > axes);
}

Keystates genKeystate(const Controller &keys, const PlayerMenuState &pms) {
  Keystates kst;
  kst.u = keys.u;
  kst.d = keys.d;
  kst.l = keys.l;
  kst.r = keys.r;
  kst.fire = keys.keys[pms.buttons[BUTTON_ACCEPT]];
  kst.change = keys.keys[pms.buttons[BUTTON_CANCEL]];
  kst.axmode = pms.setting_axistype;
  for(int j = 0; j < 2; j++) {
    kst.ax[j] = keys.axes[pms.axes[j]];
    if(pms.axes_invert[j])
      kst.ax[j] *= -1;
  }
  kst.udlrax[0] = keys.menu.x;
  kst.udlrax[1] = keys.menu.y;
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

class RenderInfo {
public:
  // Basic math!
  // compiletime constants
  static const int textline_size = 7;  // How many times larger a line of text is than its divider
  static const int textline_count = 8; // How many lines we're going to have
  static const int border_size = 2;
  static const int divider_size = 3;
  static const int units = textline_size * textline_count + textline_count - 1 + border_size * 2 + divider_size; // X lines, plus dividers (textline_count-1), plus the top and bottom borders, plus the increased divider from categories to data

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

struct StandardButtonTickData {
  vector<int> *outkeys;
  vector<char> *outinvert;
  
  int *current_button;
  bool *current_mode;
  
  Controller keys;
  const vector<float> *triggers;
  
  PlayerMenuState *pms;
};

struct StandardButtonRenderData {
  const RenderInfo *rin;
  
  const vector<int> *buttons;
  const vector<char> *inverts;
  
  const vector<vector<string> > *names;
  
  int sel_button;
  bool sel_button_reading;
  
  float fadeFactor;
  char prefixchar;
};

void standardButtonTick(StandardButtonTickData *sbtd) {
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
        for(j = 0; j < sbtd->outkeys->size(); j++) {
          if((*sbtd->outkeys)[j] == i) {
            if((*sbtd->outkeys)[*sbtd->current_button] != -1) {
              swap((*sbtd->outkeys)[j], (*sbtd->outkeys)[*sbtd->current_button]);
              if(sbtd->outinvert) {
                swap((*sbtd->outinvert)[j], (*sbtd->outinvert)[*sbtd->current_button]);
                (*sbtd->outinvert)[*sbtd->current_button] = ((*sbtd->triggers)[i] < 0);
              }
            } else {
              noopify = true;
            }
            break;
          }
        }
        if(noopify)
          continue;
        if(j == BUTTON_LAST) {
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
    if(*sbtd->current_button >= sbtd->outkeys->size())
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
  int linesneeded = 0;
  for(int i = 0; i < sbrd.names->size(); i++)
    linesneeded += (*sbrd.names)[i].size();
  CHECK(linesneeded <= sbrd.rin->ystarts.size() - 1);
  int cy = 1 + (sbrd.rin->ystarts.size()  - linesneeded + 1) / 2;
  for(int i = 0; i < (*sbrd.names).size(); i++) {
    if(sbrd.sel_button == i && !sbrd.sel_button_reading) {
      setColor(Color(1.0, 1.0, 1.0) * sbrd.fadeFactor);
    } else {
      setColor(Color(0.5, 0.5, 0.5) * sbrd.fadeFactor);
    }
    for(int j = 0; j < (*sbrd.names)[i].size(); j++)
      drawText((*sbrd.names)[i][j], sbrd.rin->textsize, sbrd.rin->xstart, sbrd.rin->ystarts[cy++]);
    string btext;
    if(sbrd.sel_button == i && sbrd.sel_button_reading) {
      btext = "???";
      setColor(Color(1.0, 1.0, 1.0) * sbrd.fadeFactor);
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
      setColor(Color(0.5, 0.5, 0.5) * sbrd.fadeFactor);
    }
    drawJustifiedText(btext.c_str(), sbrd.rin->textsize, sbrd.rin->xend, sbrd.rin->ystarts[cy - 1], TEXT_MAX, TEXT_MIN);
  }
  CHECK(cy <= sbrd.rin->ystarts.size());
}

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions) {
  if(!pms->faction) { // if player hasn't chosen faction yet
    pms->fireHeld = 0;
    {
      Float2 dir = deadzone(keys.menu, 0, 0.2) * 0.01;
      dir.y *= -1;
      pms->compasspos += dir;
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
      sbtd.keys = keys;
      sbtd.triggers = &triggers;
      sbtd.pms = pms;
      
      standardButtonTick(&sbtd);
    } else if(pms->settingmode == SETTING_AXISTYPE) {
      if(keys.l.push)
        pms->traverse_axistype(-1, keys.axes.size());
      if(keys.r.push)
        pms->traverse_axistype(1, keys.axes.size());
      if(pms->choicemode == CHOICE_ACTIVE) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push || keys.keys[pms->buttons[BUTTON_CANCEL]].push || keys.u.push) {
          pms->choicemode = CHOICE_IDLE;
        }
      } else if(pms->choicemode == CHOICE_FIRSTPASS) {
        if(keys.keys[pms->buttons[BUTTON_ACCEPT]].push) {
          pms->settingmode++;
        }
      } else {
        CHECK(0);
      }
    } else if(pms->settingmode == SETTING_AXISCHOOSE) {
      vector<float> triggers;
      for(int i = 0; i < keys.axes.size(); i++)
        triggers.push_back(keys.axes[i]);
      
      vector<int> groups(ksax_axis_names[pms->setting_axistype].size(), 0);
      
      StandardButtonTickData sbtd;      
      sbtd.outkeys = &pms->axes;
      sbtd.outinvert = &pms->axes_invert;
      sbtd.current_button = &pms->setting_axis_current;
      sbtd.current_mode = &pms->setting_axis_reading;
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
  
  // this is kind of hacky
  if(pms->settingmode == SETTING_TEST && !pms->test_game) {
    CHECK(!pms->test_player);
    
    pms->test_player = new Player(pms->faction->faction, 0);
    
    const RenderInfo rin(pms->faction->compass_location);
    
    Float4 boundy = Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]);
    boundy -= boundy.midpoint();
    
    float mn = min(boundy.x_span(), boundy.y_span());
    boundy *= (100 / mn);
    
    pms->test_game = new Game();
    pms->test_game->initDemo(pms->test_player, boundy);
  }
  
  // so is this
  if(pms->settingmode == SETTING_TEST) {
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
}

void runSettingRender(const PlayerMenuState &pms) {
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
      
      drawJustifiedMultiText(text, rin.textsize, rin.unitsize, rin.drawzone.midpoint(), TEXT_CENTER, TEXT_CENTER);
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
      drawLineLoop(rectish, 0.002);
    }
    
    {
      // Topic line!
      setColor(pms.faction->faction->color * fadeFactor);
      drawDvec2(pms.faction->faction->icon, Float4(rin.xstart, rin.ystarts[0], rin.xstart + rin.textsize, rin.ystarts[0] + rin.textsize), 50, 0.003);
      
      const int activescale = 4;
      float txstart = rin.xstart + rin.textsize + rin.border * 2;
      float title_units = (rin.xend - txstart) / (SETTING_LAST - 1 + activescale);
      
      int units = 0;
    
      for(int i = 0; i < SETTING_LAST; i++) {
        bool active = (i == pms.settingmode);
        
        int tunits = active ? activescale : 1;
        string text = active ? setting_names[i] : string() + setting_names[i][0];
        setColor(((active && pms.choicemode == CHOICE_IDLE) ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5)) * fadeFactor);
        
        drawJustifiedText(text, rin.textsize, title_units * (units + tunits / 2.) + txstart, rin.ystarts[0], TEXT_CENTER, TEXT_MIN);
        
        units += tunits;
      }
      
      CHECK(units == SETTING_LAST - 1 + activescale);
      
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
      sbrd.sel_button = pms.setting_button_current;
      sbrd.sel_button_reading = pms.setting_button_reading;
      sbrd.fadeFactor = fadeFactor;
      sbrd.prefixchar = 'B';
      
      standardButtonRender(sbrd);
    } else if(pms.settingmode == SETTING_AXISTYPE) {
      if(pms.choicemode != CHOICE_IDLE)
        setColor(Color(1.0, 1.0, 1.0) * fadeFactor);
      else
        setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      drawJustifiedText(ksax_names[pms.setting_axistype], rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[2], TEXT_CENTER, TEXT_MIN);
      
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      drawJustifiedText(ksax_descriptions[pms.setting_axistype][0], rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[4], TEXT_CENTER, TEXT_MIN);
      drawJustifiedText(ksax_descriptions[pms.setting_axistype][1], rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[5], TEXT_CENTER, TEXT_MIN);
      
      drawJustifiedText("l/r changes mode", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      
      // TODO: better pictorial representations
    } else if(pms.settingmode == SETTING_AXISCHOOSE) {
      CHECK(ksax_axis_names[pms.setting_axistype].size() == BUTTON_LAST);
      
      StandardButtonRenderData sbrd;
      sbrd.rin = &rin;
      sbrd.buttons = &pms.axes;
      sbrd.inverts = &pms.axes_invert;
      sbrd.names = &ksax_axis_names[pms.setting_axistype];
      sbrd.sel_button = pms.setting_axis_current;
      sbrd.sel_button_reading = pms.setting_axis_reading;
      sbrd.fadeFactor = fadeFactor;
      sbrd.prefixchar = 'X';
      
      standardButtonRender(sbrd);
    } else if(pms.settingmode == SETTING_TEST) {
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      if(pms.choicemode == CHOICE_IDLE) {
        drawJustifiedText("Fire to test", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      } else {
        drawJustifiedText("Cancel when done", rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[7], TEXT_CENTER, TEXT_MIN);
      }
      GfxWindow gfxw(Float4(rin.xstart, rin.ystarts[1], rin.xend, rin.ystarts[7]));
      pms.test_game->renderToScreen();
    } else if(pms.settingmode == SETTING_READY) {
      setColor(Color(0.5, 0.5, 0.5) * fadeFactor);
      const char * const text[] = {"Hold fire when", "ready. Let go", "to cancel.", "", "Move left/right to", "change config."};
      for(int i = 0; i < 6; i++)
        drawJustifiedText(text[i], rin.textsize, (rin.xstart + rin.xend) / 2, rin.ystarts[i + 2], TEXT_CENTER, TEXT_MIN);
    } else {
      CHECK(0);
    }
  }
}
