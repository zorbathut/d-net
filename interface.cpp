
#include "interface.h"

#include "ai.h"
#include "args.h"
#include "debug.h"
#include "game_tank.h"
#include "gfx.h"
#include "inputsnag.h"
#include "metagame.h"
#include "os.h"
#include "player.h"
#include "audio.h"
#include "adler32.h"
#include "game_ai.h"

#include <boost/assign.hpp>
#include <boost/bind.hpp>

using namespace std;
using boost::bind;

DEFINE_int(rounds_per_shop, 6, "How many rounds between each buying-things opportunity");
DEFINE_bool(auto_newgame, false, "Automatically enter New Game");
DEFINE_float(startingPhase, -1, "Starting phase override");
DEFINE_bool(showtanks, false, "Show-tank mode");
DEFINE_bool(dumpTanks, false, "Dump-tank mode");
DEFINE_bool(showGlobalErrors, true, "Display global errors");

DEFINE_int(factionMode, 3, "Faction mode to skip faction choice battle, -1 for normal faction mode");

StdMenuItem StdMenuItem::makeTrigger(const string &text, int trigger) {
  StdMenuItem stim;
  stim.type = TYPE_TRIGGER;
  stim.name = text;
  stim.trigger = trigger;
  return stim;
}

StdMenuItem StdMenuItem::makeScale(const string &text, const vector<string> &labels, Coord *position, const function<Coord (const Coord &)> &munge) {
  StdMenuItem stim;
  stim.type = TYPE_SCALE;
  stim.name = text;
  stim.scale_labels = labels;
  stim.scale_posfloat = position;
  stim.scale_munge = munge;
  stim.scale_posint = NULL;
  return stim;
}

StdMenuItem StdMenuItem::makeRounds(const string &text, Coord *start, Coord *end, Coord *exp) {
  StdMenuItem stim;
  stim.type = TYPE_ROUNDS;
  stim.name = text;
  stim.rounds_start = start;
  stim.rounds_end = end;
  stim.rounds_exp = exp;
  return stim;
}

StdMenuItem StdMenuItem::makeOptions(const string &text, const vector<string> &labels, int *position) {
  StdMenuItem stim;
  stim.type = TYPE_SCALE;
  stim.name = text;
  stim.scale_labels = labels;
  stim.scale_posfloat = NULL;
  stim.scale_posint = position;
  stim.scale_posint_approx = *position;
  return stim;
}

StdMenuItem StdMenuItem::makeSubmenu(const string &text, StdMenu menu) {
  StdMenuItem stim;
  stim.type = TYPE_SUBMENU;
  stim.name = text;
  stim.submenu = menu;
  return stim;
}

StdMenuItem StdMenuItem::makeBack(const string &text) {
  StdMenuItem stim;
  stim.type = TYPE_BACK;
  stim.name = text;
  return stim;
}

int StdMenuItem::tickEntire(const Keystates &keys) {
  CHECK(type == TYPE_SUBMENU);
  return submenu.tick(keys);
}

void StdMenuItem::renderEntire(const Float4 &bounds, bool obscure) const {
  CHECK(type == TYPE_SUBMENU);
  submenu.render(bounds, obscure);
}

int StdMenuItem::tickItem (const Keystates &keys) {
  if(type == TYPE_TRIGGER) {
    if(keys.accept.push) {
      queueSound(S::accept);
      return trigger;
    }
  } else if(type == TYPE_SUBMENU) {
    if(keys.accept.push) {
      queueSound(S::accept);
      submenu.reset();
      return SMR_ENTER;
    }
  } else if(type == TYPE_BACK) {
    if(keys.accept.push) {
      queueSound(S::choose);
      return SMR_RETURN;
    }
  } else if(type == TYPE_SCALE) {
    if(scale_posfloat) {
      if(keys.l.down)
        *scale_posfloat -= Coord(1) / 16;
      if(keys.r.down)
        *scale_posfloat += Coord(1) / 16;
      if(keys.l.down || keys.r.down)
        *scale_posfloat = clamp(*scale_posfloat, 0, scale_labels.size() - 1);
    } else {
      CHECK(scale_posint);
      if(keys.l.down)
        scale_posint_approx -= Coord(1) / 32;
      if(keys.r.down)
        scale_posint_approx += Coord(1) / 32;
      if(!keys.l.down && !keys.r.down)
        scale_posint_approx = approach(scale_posint_approx, round(scale_posint_approx), Coord(1) / 128);
      scale_posint_approx = clamp(scale_posint_approx, 0, scale_labels.size() - 1);
      *scale_posint = round(scale_posint_approx).toInt();
    }
  } else if(type == TYPE_ROUNDS) {
    if(keys.l.down)
      *rounds_exp *= Coord(101) / 100;
    if(keys.r.down)
      *rounds_exp /= Coord(101) / 100;
    *rounds_exp = clamp(*rounds_exp, Coord(0.001), 2);
  } else {
    CHECK(0);
  }
  return SMR_NOTHING; 
}

int calculateRounds(Coord start, Coord end, Coord exp) {
  return floor(ceil((end - start) * log(30) / exp / 6)).toInt() * 6;
}

float StdMenuItem::renderItemHeight() const {
  if(type == TYPE_TRIGGER || type == TYPE_SCALE || type == TYPE_ROUNDS || type == TYPE_SUBMENU) {
    return 6;
  } else {
    CHECK(0);
  }
}

void StdMenuItem::renderItem(const Float4 &bounds) const {
  if(type == TYPE_TRIGGER || type == TYPE_SUBMENU || type == TYPE_BACK) {
    drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
  } else if(type == TYPE_SCALE) {
    drawText(name.c_str(), 4, bounds.s());
    
    CHECK(!scale_posfloat + !scale_posint == 1);
    float pos;
    if(scale_posfloat)
      pos = scale_posfloat->toFloat();
    else
      pos = scale_posint_approx.toFloat();
    
    Float4 boundy = Float4(bounds.sx + 35, bounds.sy, bounds.ex, bounds.sy + 4);
    GfxWindow gfxw(boundy, 1.0);
    
    setZoomAround(Float4(pos - 2, 0, pos + 2, 0));
    
    float height = getZoom().span_y();
    
    for(int i = 0; i < scale_labels.size(); i++) {
      setColor(C::active_text);
      drawJustifiedText(scale_labels[i], height / 2 * 0.9, Float2(i, 0), TEXT_CENTER, TEXT_MAX);
      setColor(C::inactive_text);
      drawLine(Float4(i, height / 2 - height / 6, i, height / 6), height / 20);
    }

    setColor(C::inactive_text);
    drawLine(Float4(0, height / 4, scale_labels.size() - 1, height / 4), height / 20);
    
    setColor(C::active_text);
    {
      vector<Float2> path;
      path.push_back(Float2(pos, height / 8));
      path.push_back(Float2(pos + height / 16, height / 4));
      path.push_back(Float2(pos, height / 2 - height / 8));
      path.push_back(Float2(pos - height / 16, height / 4));
      drawLineLoop(path, height / 20);
    }
  } else if(type == TYPE_ROUNDS) {
    drawText(name.c_str(), 4, bounds.s());
    float percentage = (exp(rounds_exp->toFloat()) - 1) * 100;
    drawText(StringPrintf("%d (+%.2f%% cash/round)", calculateRounds(*rounds_start, *rounds_end, *rounds_exp), percentage), 4, Float2(bounds.sx + 50, bounds.sy));
  } else {
    CHECK(0);
  }
}

StdMenuItem::StdMenuItem() { }

void StdMenu::pushMenuItem(const StdMenuItem &site) {
  items.push_back(site);
}

int StdMenu::tick(const Keystates &keys) {
  if(!inside) {
    if(keys.u.repeat) {
      cpos--;
      queueSound(S::select);
    }
    if(keys.d.repeat) {
      cpos++;
      queueSound(S::select);
    }
    cpos = modurot(cpos, items.size());
  }
  
  for(int i = 0; i < items.size(); i++)
    if(i != cpos)
      items[i].tickItem(Keystates());
    
  {
    int rv;
    if(inside)
      rv = items[cpos].tickEntire(keys);
    else
      rv = items[cpos].tickItem(keys);
    
    if(rv == SMR_NOTHING) {
      return SMR_NOTHING;
    } else if(rv == SMR_ENTER) {
      inside = true;
      return SMR_NOTHING;
    } else if(rv == SMR_RETURN && !inside) {
      return SMR_RETURN;
    } else if(rv == SMR_RETURN && inside) {
      inside = false;
      return SMR_NOTHING;
    } else {
      return rv;
    }
  }
}

void StdMenu::render(const Float4 &bounds, bool obscure) const {
  CHECK(!obscure);
  
  if(inside) {
    items[cpos].renderEntire(bounds, obscure);
  } else {
    GfxWindow gfxw(bounds, 1.0);
    
    float totheight = 0;
    for(int i = 0; i < items.size(); i++)
      totheight += items[i].renderItemHeight();
    
    setZoomCenter(0, 0, getZoom().span_y() / 2);
    
    float curpos = (getZoom().span_y() - totheight) / 2;
    for(int i = 0; i < items.size(); i++) {
      if(i == cpos) {
        setColor(C::active_text);
      } else {
        setColor(C::inactive_text);
      }
      
      items[i].renderItem(Float4(getZoom().sx + 2, getZoom().sy + curpos, getZoom().ex - 2, -1));
      curpos += items[i].renderItemHeight();
    }
  }
}

void StdMenu::reset() {
  cpos = 0;
  inside = false;
}

StdMenu::StdMenu() {
  cpos = 0;
  inside = false;
}

class GameAiIntro : public GameAi{
private:
  Coord2 nextpos;
  Coord lastdist;
  int waitcycles;

  void updateGameWork(const vector<Tank> &players, int me) {
    CHECK(0);
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    if(waitcycles > 0) {
      waitcycles--;
      return;
    }
    
    if(nextpos == Coord2(-1000, -1000))
      genNewPos(mypos);
    
    nextKeys.udlrax = normalize(nextpos - mypos);
    nextKeys.udlrax.y *= -1;
    
    Coord tdist = len(nextpos - mypos);
    if(tdist > lastdist) {
      nextKeys.fire[0].down = true;
      genNewPos(mypos);
    } else {
      lastdist = tdist;
    }
  }
  
  void genNewPos(Coord2 cpos) {
    int divert = 50;
    float mag;
    do {
      mag = unsync().frand();
      nextpos = cpos + makeAngle(unsync().frand() * COORDPI * 2) * mag * divert;
      divert += 10;
    } while(!isInside(Coord4(-364, -116, 364, 0), nextpos) && !isInside(Coord4(-112, 0, 112, 92), nextpos));
    lastdist = 1000;
  }
  
public:
  GameAiIntro() {
    nextpos = Coord2(-1000, -1000);
    waitcycles = unsync().choose(FPS * 4);
  }
  
  
};

bool InterfaceMain::tick(const InputState &is, RngSeed gameseed) {
  if(is.escape.push) {
    escmenu = !escmenu;
    if(escmenu)
      escmenuitem.reset();
  }
  
  if(FLAGS_showtanks)
    return false;
  
  StackString stp("Interface ticking");
  
  inptest_controls = is.controllers;
  
  if(kst.size() == 0) {
    CHECK(is.controllers.size() != 0);
    kst.resize(is.controllers.size());
  }
  
  CHECK(kst.size() == is.controllers.size());
  
  for(int i = 0; i < is.controllers.size(); i++) {
    CHECK(is.controllers[i].keys.size() >= 1);
    kst[i].u.newState(is.controllers[i].menu.y > .5);
    kst[i].d.newState(is.controllers[i].menu.y < -.5);
    kst[i].r.newState(is.controllers[i].menu.x > .5);
    kst[i].l.newState(is.controllers[i].menu.x < -.5);
    bool aButtonPushed = false;
    for(int j = 0; j < is.controllers[i].keys.size(); j++)
      if(is.controllers[i].keys[j].push)
        aButtonPushed = true;
    kst[i].accept.newState(aButtonPushed);
    kst[i].cancel.newState(false);
    for(int j = 0; j < SIMUL_WEAPONS; j++)
      kst[i].fire[j].newState(false);
  }
  
  if(escmenu) {
    int rv = escmenuitem.tick(kst[controls_primary_id()]);
    if(rv == 0)
      escmenu = false;
    if(rv == 1)
      return true;
  } else {
    if(interface_mode == STATE_MAINMENU) {
      if(!inptest) {
        introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
        introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
      }
      
      int mrv = mainmenu.tick(kst[controls_primary_id()]);
      if(mrv == MAIN_NEWGAME || FLAGS_auto_newgame) {
        if(faction_toggle == 0)
          faction = 4;
        else
          faction = 1;
        game = new Metagame(kst.size(), Money((long long)(1000 * pow(30, start.toFloat()))), exp(moneyexp), faction - 1, FLAGS_rounds_per_shop, calculateRounds(start, end, moneyexp), gameseed);
        interface_mode = STATE_PLAYING;
      } else if(mrv == MAIN_EXIT) {
        return true;
      } else if(mrv == MAIN_INPUTTEST) {
        inptest = !inptest;
      } else if(mrv == MAIN_GRID) {
        grid = !grid;
      } else {
        CHECK(mrv == -1);
      }
    } else if(interface_mode == STATE_PLAYING) {
      if(game->runTick(is.controllers)) {
        init();   // full reset
      }
    } else {
      CHECK(0);
    }
  }
  
  return false;
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
  StackString stp("Interface AI");
  if(interface_mode == STATE_MAINMENU) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updatePregame();
  } else if(interface_mode == STATE_PLAYING) {
    game->ai(ai);
  } else {
    CHECK(0);
  }
}

bool tankCostSorter(const pair<string, IDBTank> &lhs, const pair<string, IDBTank> &rhs) {
  //return lhs.first < rhs.first;
  if(lhs.second.base_cost != rhs.second.base_cost)
    return lhs.second.base_cost < rhs.second.base_cost;
  return lhs.second.engine > rhs.second.engine;
}

Coord InterfaceMain::start_clamp(const Coord &opt) const {
  return clamp(opt, 0, end - 0.1);
}
Coord InterfaceMain::end_clamp(const Coord &opt) const {
  return clamp(opt, start + 0.1, 8);
}

void InterfaceMain::render() const {
  if(isUnoptimized()) {
    setZoom(Float4(0, 0, 133.3333, 100));
    setColor(Color(1.0, 0.3, 0.3));
    drawText("Optimizations disabled!", 6, Float2(1, 1));
  }
  
  if(FLAGS_dumpTanks) {  // this should probably be in main.cpp really
    static bool dumpedtanks = false;
    
    if(!dumpedtanks) {
      vector<pair<string, IDBTank> > ttk;
      for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++)
        ttk.push_back(make_pair(itr->first, itr->second));
      sort(ttk.begin(), ttk.end(), tankCostSorter);
      
      dumpedtanks = true;
      
      int x = 0;
      int y = 0;
      
      bool addedfinal = false;
      FILE *of = fopen("tanks.dv2", "w");
      for(int i = 0; i < ttk.size(); i++) {
        fprintf(of, "path {\n  center=%d,%d\n  reflect=spin\n  dupes=1\n  angle=0/1\n", x, y);
        if(i % 3 == 0) {
          x = -1000;
          y += 1000;
          if(i % 6 == 0)
            y += 1000;
        } else {
          x += 1000;
        }
        for(int j = 0; j < ttk[i].second.vertices.size(); j++)
          fprintf(of, "  node= --- | %f,%f | ---\n", (ttk[i].second.vertices[j].y + ttk[i].second.centering_adjustment.y).toFloat() * 100, -(ttk[i].second.vertices[j].x + ttk[i].second.centering_adjustment.x).toFloat() * 100);
        fprintf(of, "}\n\n");
        if(i + 1 == ttk.size() && !addedfinal) {
          i -= 6; // hee hee
          addedfinal = true;
        }
      }
      fclose(of);
    }
  }
  
  if(FLAGS_showtanks) {
    setZoomVertical(0, 0, 200);
    setColor(1.0, 1.0, 1.0);
    float x = 0;
    float y = 0;
    const float xsize = 30;
    const float ysize = 30;
    vector<pair<string, IDBTank> > ttk;
    for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++)
      ttk.push_back(make_pair(itr->first, itr->second));
    sort(ttk.begin(), ttk.end(), tankCostSorter);
    for(int i = 0; i < ttk.size(); i++) {
      vector<Float2> vertices;
      for(int j = 0; j < ttk[i].second.vertices.size(); j++)
        vertices.push_back(Float2(ttk[i].second.vertices[j].y.toFloat(), -ttk[i].second.vertices[j].x.toFloat()) + Float2(xsize, ysize) / 2 + Float2(x, y));
      drawLineLoop(vertices, 0.5);
      drawJustifiedText(strrchr(ttk[i].first.c_str(), '.') + 1, 3, Float2(xsize / 2, ysize - 1) + Float2(x, y), TEXT_CENTER, TEXT_MAX);
      x += xsize;
      if(x + xsize > getZoom().ex) {
        y += ysize;
        x = 0;
      }
    }
    return;
  }
  
  StackString stp("Interface rendering");
  
  {
    smart_ptr<GfxWindow> wnd;
    if(escmenu)
      wnd.reset(new GfxWindow(getZoom(), 0.5));
  
    if(FLAGS_showGlobalErrors) {
      for(int i = 0; i < returnErrorMessages().size(); i++) {
        setZoomCenter(0, 0, 100);
        setColor(1.0, 1.0, 1.0);
        drawJustifiedText(returnErrorMessages()[i], 30, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
      }
    }
    
    if(interface_mode == STATE_MAINMENU) {
      setZoomVertical(0, 0, 100);
      mainmenu.render(Float4(5, 50, getZoom().ex - 5, 95), false);
      setColor(C::inactive_text * 0.5);
      drawJustifiedText("Use the arrow keys to choose menu items. Hit Enter to select.", 3, Float2(getZoom().midpoint().x, getZoom().ey - 3), TEXT_CENTER, TEXT_MAX);
      if(grid) {
        setColor(1.0, 1.0, 1.0);
        drawGrid(1, 0.01);
      }
      if(inptest) {
        setZoom(Float4(0, 0, 800, 600));
        setColor(1.0, 1.0, 1.0);
        const float xsiz = 100;
        const float bord = xsiz / 25;
        const float usablesiz = xsiz - bord * 4;
        const float crosshair = usablesiz / 2;
        const float textheight = 9;
        const float ysiz = crosshair + bord * 3 + textheight;
        const float crosshairc = crosshair / 2;
        const float textsize = crosshair / 4;
        const float textrsize = textsize * 0.8;
        const int wid = int(700 / xsiz);
        const int textymax = int(crosshair / textsize);
        const int textxmax = 2;
        const float textxofs = crosshair / textxmax;
        
        const float boxthick = 1.0;
        for(int i = 0; i < inptest_controls.size(); i++) {
          const Controller &ct = inptest_controls[i];
          float x = (i % wid) * xsiz + 50;
          float y = (i / wid) * ysiz + 50;
          if(i % wid == 0 && i) {
            setColor(C::box_border);
            drawLine(Float4(50, y, 750, y), boxthick);
          }
          setColor(C::gray(1.0));
          drawJustifiedText(controls_getcc(i).description, textheight, Float2(x + xsiz / 2, y + bord), TEXT_CENTER, TEXT_MIN);
          Float4 chbox(x + bord, y + bord * 2 + textheight, x + bord + crosshair, y + bord * 2 + crosshair + textheight);
          setColor(C::box_border);
          drawLine(Float4(chbox.sx, chbox.sy, chbox.sx + crosshair / 4, chbox.sy), boxthick);
          drawLine(Float4(chbox.sx, chbox.sy, chbox.sx, chbox.sy + crosshair / 4), boxthick);
          drawLine(Float4(chbox.sx, chbox.ey, chbox.sx + crosshair / 4, chbox.ey), boxthick);
          drawLine(Float4(chbox.sx, chbox.ey, chbox.sx, chbox.ey - crosshair / 4), boxthick);
          drawLine(Float4(chbox.ex, chbox.sy, chbox.ex - crosshair / 4, chbox.sy), boxthick);
          drawLine(Float4(chbox.ex, chbox.sy, chbox.ex, chbox.sy + crosshair / 4), boxthick);
          drawLine(Float4(chbox.ex, chbox.ey, chbox.ex - crosshair / 4, chbox.ey), boxthick);
          drawLine(Float4(chbox.ex, chbox.ey, chbox.ex, chbox.ey - crosshair / 4), boxthick);
          setColor(C::inactive_text);
          drawCrosshair(Coord2(ct.menu.x * crosshairc + bord + crosshairc + x, -ct.menu.y * crosshairc + bord * 2 + crosshairc + y + textheight), crosshair / 4, boxthick);
          setColor(C::active_text);
          float textx = x + bord * 3 + crosshair;
          float texty = y + bord * 2 + textheight;
          int ctxt = 0;
          int kd = 0;
          for(int j = 0; j < ct.keys.size(); j++)
            if(ct.keys[j].down)
              kd++;
          for(int j = 0; j < ct.keys.size(); j++) {
            if(ct.keys[j].down) {
              string tbd;
              if(ctxt >= textymax * textxmax) {
                continue;
              } else if(ctxt == textymax * textxmax - 1 && kd > textymax * textxmax) {
                tbd = "...";
              } else {
                tbd = StringPrintf("%d", j);
              }
              if(tbd != "") {
                drawText(tbd, textrsize, Float2(textx + ctxt / textymax * textxofs, texty + (ctxt % textymax) * textsize));
                ctxt++;
              }
            }
          }
        }
      } else {
        GfxWindow gfxw(Float4(0, 0, getZoom().ex, 60), 2.0);
        introscreen->renderToScreen();
      }
    } else if(interface_mode == STATE_PLAYING) {
      game->renderToScreen();
    } else {
      CHECK(0);
    }
  }
  
  if(escmenu) {
    setZoomVertical(0, 0, 100);
    escmenuitem.render(boxAround(getZoom().midpoint(), 40), false);
  }
};

void InterfaceMain::checksum(Adler32 *adl) const {
  adler(adl, interface_mode);
  adler(adl, start);
  adler(adl, end);
  adler(adl, moneyexp);
  adler(adl, faction);
  
  //reg_adler_intermed(*adl);
  
  if(game)
    game->checksum(adl);
}

void InterfaceMain::init() {
  mainmenu = StdMenu();
  kst.clear();
  introscreen_ais.clear();
  delete introscreen;
  introscreen = new GamePackage;
  
  interface_mode = STATE_MAINMENU;
  {
    vector<string> names = boost::assign::list_of("Junkyard")("Civilian")("Professional")("Military")("Exotic")("Experimental")("Ultimate");
    
    StdMenu configmenu;
    configmenu.pushMenuItem(StdMenuItem::makeScale("Game start", names, &start, bind(&InterfaceMain::start_clamp, this, _1)));
    names.push_back("Armageddon");
    configmenu.pushMenuItem(StdMenuItem::makeScale("Game end", names, &end, bind(&InterfaceMain::end_clamp, this, _1)));
    configmenu.pushMenuItem(StdMenuItem::makeRounds("Estimated rounds", &start, &end, &moneyexp));
    configmenu.pushMenuItem(StdMenuItem::makeOptions("Factions", boost::assign::list_of("On")("Off"), &faction_toggle));
    //configmenu.pushMenuItem(StdMenuItem::makeOptions("Faction mode", boost::assign::list_of("Battle")("No factions")("Minor factions")("Normal factions")("Major factions"), &faction));
    configmenu.pushMenuItem(StdMenuItem::makeTrigger("Begin", MAIN_NEWGAME));
    
    mainmenu.pushMenuItem(StdMenuItem::makeSubmenu("New game", configmenu));
  }
  
  mainmenu.pushMenuItem(StdMenuItem::makeTrigger("Input test", MAIN_INPUTTEST));
  mainmenu.pushMenuItem(StdMenuItem::makeTrigger("Exit", MAIN_EXIT));
  
  if(FLAGS_startingPhase == -1)
    start = 0;
  else
    start = Coord(FLAGS_startingPhase);
  end = 7;
  moneyexp = Coord(0.1133);
  
  faction = FLAGS_factionMode + 1;
  CHECK(faction >= 0 && faction < 5);
  
  if(faction == 1)
    faction_toggle = 1;
  else
    faction_toggle = 0;
  
  grid = false;
  inptest = false;
  game = NULL;
  
  {
    introscreen->players.resize(16);
    vector<const IDBFaction *> idbfa = ptrize(factionList());
    for(int i = 0; i < introscreen->players.size(); i++) {
      int fid = unsync().choose(idbfa.size());
      introscreen->players[i] = Player(idbfa[fid], 0, Money(0));
      idbfa.erase(idbfa.begin() + fid);
    }
  }
  
  introscreen->game.initTitlescreen(&introscreen->players, &unsync());
  
  for(int i = 0; i < introscreen->players.size(); i++)
    introscreen_ais.push_back(new GameAiIntro());
  
  escmenuitem.pushMenuItem(StdMenuItem::makeBack("Return to game"));
  escmenuitem.pushMenuItem(StdMenuItem::makeTrigger("Main menu", 1));
  escmenuitem.pushMenuItem(StdMenuItem::makeTrigger("Quit", 2));
  
  for(int i = 0; i < 180; i++)
    introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
}
InterfaceMain::InterfaceMain() {
  introscreen = NULL;
  escmenu = false;
  init();
}

InterfaceMain::~InterfaceMain() {
  dprintf("Deleting metagame\n");
  delete game;
  dprintf("Metagame deleted\n");
}

