
#include "interface.h"

#include "ai.h"
#include "dumper_registry.h"
#include "game_projectile.h"
#include "game_tank.h"
#include "gfx.h"
#include "os.h"
#include "res_interface.h"
#include "smartptr_util.h"
#include "version.h"

#include <numeric>

#include <boost/assign.hpp>
#include <boost/bind.hpp>

using namespace std;

using boost::bind;

DEFINE_int(rounds_per_shop, 6, "How many rounds between each buying-things opportunity");
DEFINE_bool(auto_newgame, false, "Automatically enter New Game");
DEFINE_bool(showtanks, false, "Show-tank mode");
DEFINE_bool(dumpTanks, false, "Dump-tank mode");
DEFINE_bool(showGlobalErrors, true, "Display global errors");

DEFINE_int(factionMode, 3, "Faction mode to skip faction choice battle, -1 for normal faction mode");
DEFINE_float(startingPhase, -1, "Starting phase override");
DEFINE_bool(demolimits, false, "Demo mode limits");

REGISTER_int(rounds_per_shop);
REGISTER_bool(auto_newgame);
REGISTER_int(factionMode);
REGISTER_float(startingPhase);
REGISTER_bool(demolimits);

/*************
 * GameAiIntro
 */

class GameAiIntro : public GameAi {
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

/*************
 * Interface
 */

bool InterfaceMain::tick(const InputState &is, RngSeed gameseed, InputSnag &isnag) {
  if(is.escape.push) {
    inescmenu = !inescmenu;
    if(inescmenu)
      escmenu.reset();
  }
  
  mouseconf_cooldown--;
  mouseconf_cooldown = max(mouseconf_cooldown, 0);
  if(is.confused_mouse)
    mouseconf_cooldown = FPS / 6;
  
  if(FLAGS_showtanks)
    return false;
  
  StackString stp("Interface ticking");
  
  inptest_controls = is.controllers;
  
  kst.resize(is.controllers.size());  // yeah we'll throw them away and bring 'em back as new controllers show up and vanish
  
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
  
  if(inescmenu) {
    pair<StdMenuCommand, int> rv = escmenu.tick(kst[isnag.primary_id()]);
    if(rv.first == SMR_RETURN)
      inescmenu = false;
    if(rv.second == SMR_NOTHING) {
    } else if(rv.second == ESCMENU_MAINMENU) {
      dprintf("re-initting\n");
      inescmenu = false;
      interface_mode = STATE_MAINMENU;
      init(isnag);
    } else if(rv.second == ESCMENU_QUIT) {
      return true;
    } else if(rv.second == ESCMENU_ENDGAME) {
      inescmenu = false;
      game->endgame();
      if(game->runTick(is.controllers, is.confused, isnag)) {
        init(isnag);   // full reset
      }
    } else if(rv.second == OPTS_SETRES) {
      setResolution(opts_res, opts_aspect, opts_fullscreen);
    } else {
      CHECK(0);
    }
  } else if(interface_mode == STATE_DISCLAIMER) {
    bool pushed = false;
    for(int i = 0; i < is.controllers.size(); i++)
      for(int j = 0; j < is.controllers[i].keys.size(); j++)
        if(is.controllers[i].keys[j].push)
          pushed = true;
    
    if(is.confused)
      pushed = true;
    
    if(is.confused_mouse)
      pushed = true;
    
    if(pushed)
      interface_mode = STATE_MAINMENU;
  } else if(interface_mode == STATE_MAINMENU) {
    if(!inptest) {
      tickIntroScreen();
    }
    
    pair<StdMenuCommand, int> mrv = mainmenu.tick(kst[isnag.primary_id()]);
    
    bool instantaction = false;
    if(mrv.second == MAIN_INSTANTACTION) {
      aicount = 4;
      faction_toggle = true;
      start = 0;
      end = end_max;
      moneyexp = Coord(0.18);
      mrv.second = MAIN_NEWGAME;
      instantaction = true;
    }
    
    if(mrv.second == MAIN_NEWGAME || FLAGS_auto_newgame) {
      if(!faction_toggle)
        faction = 1;
      else
        faction = 4;
      isnag.set_ai_count(aicount); // this is pretty grim really
      game.reset(new Metagame(isnag.human_flags(), Money((long long)(1000 * pow(30, start.toFloat()))), exp(moneyexp), faction - 1, FLAGS_rounds_per_shop, rounds, gameseed, isnag));
      if(instantaction)
        game->instant_action_init(isnag.getcc(isnag.primary_id()), isnag.primary_id());
      dprintf("ENTERING PLAYING\n");
      interface_mode = STATE_PLAYING;
      escmenu = escmenuig;
    } else if(mrv.second == MAIN_EXIT) {
      return true;
    } else if(mrv.second == MAIN_INPUTTEST) {
      inptest = !inptest;
    } else if(mrv.second == MAIN_GRID) {
      grid = !grid;
    } else if(mrv.second == MAIN_NEWGAMEMENU) {
      inptest = false;
      grid = false; // :D
    } else if(mrv.second == OPTS_SETRES) {
      setResolution(opts_res, opts_aspect, opts_fullscreen);
    } else {
      CHECK(mrv.second == -1);
    }
  } else if(interface_mode == STATE_PLAYING) {
    if(game->runTick(is.controllers, is.confused, isnag)) {
      init(isnag);   // full reset
    }
  } else {
    CHECK(0);
  }
  return false;
}

void InterfaceMain::ai(const vector<Ai *> &ai, const vector<bool> &isHuman) const {
  StackString stp("Interface AI");
  if(interface_mode == STATE_DISCLAIMER) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updateIdle();
  } else if(interface_mode == STATE_MAINMENU) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updatePregame();
  } else if(interface_mode == STATE_PLAYING) {
    game->ai(ai, isHuman);
  } else {
    CHECK(0);
  }
}

bool InterfaceMain::isWaitingOnAi() const {
  if(interface_mode == STATE_PLAYING)
    return game->isWaitingOnAi();
  return false;
}

bool tankCostSorter(const pair<string, IDBTank> &lhs, const pair<string, IDBTank> &rhs) {
  if(lhs.second.base_cost != rhs.second.base_cost)
    return lhs.second.base_cost < rhs.second.base_cost;
  return lhs.second.engine > rhs.second.engine;
}

Coord InterfaceMain::start_clamp(const Coord &opt) const {
  return clamp(opt, 0, end - Coord(1) / 16);
}
Coord InterfaceMain::end_clamp(const Coord &opt) const {
  return clamp(opt, start + Coord(1) / 16, end_max);
}

int gcd(int x, int y) {
  while(y) {
    int t = y;
    y = x % y;
    x = t;
  }
  return x;
}

bool is_relatively_prime(int i, int j) {
  return gcd(i, j) == 1;
}

class rati {
public:
  bool operator()(pair<int, int> lhs, pair<int, int> rhs) {
    lhs.first *= rhs.second;
    rhs.first *= lhs.second;
    return lhs.first > rhs.first;
  }
};

vector<pair<string, float> > gen_aspects(int x, int y) {
  vector<pair<int, int> > asps;
  asps.push_back(make_pair(4, 3));
  asps.push_back(make_pair(16, 10));
  asps.push_back(make_pair(16, 9));
  
  asps.push_back(make_pair(x / gcd(x, y), y / gcd(x, y)));
  
  if(count(asps.begin(), asps.end(), make_pair(8, 5)))
    asps.erase(find(asps.begin(), asps.end(), make_pair(8, 5)));
  
  sort(asps.begin(), asps.end(), rati());
  asps.erase(unique(asps.begin(), asps.end()), asps.end());
  
  vector<pair<string, float> > rv;
  for(int i = 0; i < asps.size(); i++)
    rv.push_back(make_pair(StringPrintf("%d:%d", asps[i].first, asps[i].second), (float)asps[i].first / asps[i].second));
  
  return rv;
}

void InterfaceMain::opts_res_changed(pair<int, int> newres) {
  if(newres.second * 5 == newres.first * 4) // this isn't actually 5:4, this is probably 4:3
    newres = make_pair(640, 480);
  
  opts_aspect = (float)newres.first / newres.second;
  
  opts_aspect_chooser->changeOptionDb(gen_aspects(newres.first, newres.second));
}

void InterfaceMain::render(const InputSnag &is) const {
  if(isUnoptimized()) {
    setZoomVertical(0, 0, 100);
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
    if(inescmenu)
      wnd.reset(new GfxWindow(getZoom(), 0.5));
  
    if(FLAGS_showGlobalErrors) {
      for(int i = 0; i < returnErrorMessages().size(); i++) {
        setZoomCenter(0, 0, 100);
        setColor(1.0, 1.0, 1.0);
        drawJustifiedText(returnErrorMessages()[i], 30, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
      }
    }
    
    if(interface_mode == STATE_DISCLAIMER) {
      setZoomVertical(0, 0, 100);
      const float bord = 3;
      
      vector<string> text;
      setColor(C::inactive_text);
      text.push_back("Devastation Net is fundamentally a multiplayer game.");
      text.push_back("There is, at this point, AI for it. However the AI is still under heavy development and does not work particularly well.");
      text.push_back("It is best played with friends. It supports USB game controllers, which I highly recommend. Much like Super Smash Brothers Brawl and similar games, it is not really ideal as a singleplayer game.");
      text.push_back("Singleplayer will, however, give you a sense of the gameplay - and hopefully interest you enough to bring some friends over.");
      text.push_back("Visit www.mandible-games.com for recent versions, and thanks for playing.");
      
      string accum;
      for(int i = 0; i < text.size(); i++) {
        if(accum.size())
          accum += "\n\n";
        accum += text[i];
      }
      drawJustifiedParagraphedText(accum, 3, make_pair(getZoom().sx + bord, getZoom().ex - bord), getZoom().sy + bord, TEXT_MIN);
      
      setColor(C::active_text);
      drawJustifiedText("(press enter to continue)", 3, Float2(getZoom().midpoint().x, getZoom().ey - bord), TEXT_CENTER, TEXT_MAX);
    } else if(interface_mode == STATE_MAINMENU) {
      setZoomVertical(0, 0, 100);
      mainmenu.render(Float4(5, 50, getZoom().ex - 5, 95), false);
      setColor(C::inactive_text * 0.5);
      drawJustifiedText("Use the arrow keys to choose menu items. Hit Enter to select.", mouseconf_cooldown ? 4.5 : 3, Float2(getZoom().midpoint().x, getZoom().ey - 4.5), TEXT_CENTER, TEXT_CENTER);
      if(grid) {
        setColor(1.0, 1.0, 1.0);
        drawGrid(1, 0.01);
      }
      if(inptest) {
        setZoomVertical(0, 0, 600);
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
        const int wid = int((getZoom().ex - 100) / xsiz);
        const int textymax = int(crosshair / textsize);
        const int textxmax = 2;
        const float textxofs = crosshair / textxmax;
        
        float xmarg = (getZoom().ex - wid * xsiz) / 2;
        
        const float boxthick = 1.0;
        
        int citem = 0;
        
        for(int i = 0; i < inptest_controls.size(); i++) {
          if(!inptest_controls[i].human)
            continue;
          
          const Controller &ct = inptest_controls[i];
          float x = (citem % wid) * xsiz + xmarg;
          float y = (citem / wid) * ysiz + 50;
          if(i % wid == 0 && i) {
            setColor(C::box_border);
            drawLine(Float4(xmarg, y, getZoom().ex - xmarg, y), boxthick);
          }
          setColor(C::gray(1.0));
          drawJustifiedText(is.getcc(i).description, textheight, Float2(x + xsiz / 2, y + bord), TEXT_CENTER, TEXT_MIN);
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
          
          citem++;
        }
      } else {
        GfxWindow gfxw(Float4(0, 0, getZoom().ex, 60), 2.0);
        renderIntroScreen();
      }
      
      setColor(C::gray(0.5));
      drawText(dnet_version, 2, Float2(0.5, 0.5));
    } else if(interface_mode == STATE_PLAYING) {
      game->renderToScreen(is);
    } else {
      CHECK(0);
    }
  }
  
  if(inescmenu) {
    setZoomVertical(0, 0, 100);
    escmenu.render(getZoom(), true);
  }
  
  // testing line width
  /*
  {
    setZoomVertical(0, 0, 100);
    setColor(C::gray(0.5));
    for(int i = 0; i < 10; i++) {
      const float mult = 2;
      drawLine(Float4(10 + i * mult, 10, 10 + i * mult, 20), mult);
      drawLine(Float4(100, 10 + i * mult, 110, 10 + i * mult), mult);
    }
  }*/
};

void InterfaceMain::checksum(Adler32 *adl) const {
  mainmenu.checksum(adl);
  escmenu.checksum(adl);
  escmenuig.checksum(adl);
  optionsmenu.checksum(adl);
  adler(adl, interface_mode);
  adler(adl, start);
  adler(adl, end);
  adler(adl, moneyexp);
  adler(adl, faction);
  
  if(game)
    game->checksum(adl);
}

void InterfaceMain::forceResize(int w, int h) {
  w = max(w, 100);
  h = min(h, w / 4 * 3);
  h = max(h, w / 6);  // no, you can't have 6:1 aspect. Not allowed.
  // At this point we have our actual W/H values that we'll be using.
  
  dprintf("oa is %f\n", opts_aspect);
  float aspect_per_pixel = opts_aspect / opts_res.first * opts_res.second;
  dprintf("Resolution was %d/%d with app %f asp %f, now is %d/%d\n", opts_res.first, opts_res.second, aspect_per_pixel, opts_aspect, w, h);
  opts_res.first = w;
  opts_res.second = h;
  opts_aspect = aspect_per_pixel * w / h;
  CHECK(setResolution(make_pair(w, h), opts_aspect, opts_fullscreen));
}

void InterfaceMain::init(const InputSnag &isnag) {
  StackString stp("IMain initting");
  
  mainmenu = StdMenu();
  escmenu = StdMenu();
  escmenuig = StdMenu();
  optionsmenu = StdMenu();
  kst.clear();
  
  aicount = isnag.get_ai_count();
  
  faction = FLAGS_factionMode + 1;
  CHECK(faction >= 0 && faction < 5);
  
  if(faction == 1)
    faction_toggle = false;
  else
    faction_toggle = true;
  
  interface_mode = STATE_DISCLAIMER;
  
  mainmenu.pushMenuItem(StdMenuItemTrigger::make("Instant Action", MAIN_INSTANTACTION));
  
  {
    vector<string> names = boost::assign::list_of("Junkyard")("Civilian")("Professional")("Military")("Exotic")("Experimental")("Ultimate")("Armageddon");
    if(FLAGS_demolimits)
      names.erase(names.begin() + 3, names.end());
    
    StdMenu configmenu;
    configmenu.pushMenuItem(StdMenuItemScale::make("Game start", &start, bind(&InterfaceMain::start_clamp, this, _1), StdMenuItemScale::ScaleDisplayer(names, &start, &end, &onstart, true), true, &onstart));
    configmenu.pushMenuItem(StdMenuItemScale::make("Game end", &end, bind(&InterfaceMain::end_clamp, this, _1), StdMenuItemScale::ScaleDisplayer(names, &start, &end, &onstart, false), false, &onstart));
    configmenu.pushMenuItem(StdMenuItemRounds::make("Estimated rounds", &start, &end, &moneyexp, &rounds));
    configmenu.pushMenuItem(StdMenuItemCounter::make("AI count", &aicount, 0, max(15, aicount)));
    
    {
      vector<pair<string, bool> > onoff;
      onoff.push_back(make_pair("On", true));
      onoff.push_back(make_pair("Off", false));
      configmenu.pushMenuItem(StdMenuItemChooser<bool>::make("Factions", onoff, &faction_toggle));
    }
    
    //configmenu.pushMenuItem(StdMenuItem::makeOptions("Faction mode", boost::assign::list_of("Battle")("No factions")("Minor factions")("Normal factions")("Major factions"), &faction));
    configmenu.pushMenuItem(StdMenuItemTrigger::make("Begin", MAIN_NEWGAME));
    configmenu.pushMenuItemAdjacent(StdMenuItemBack::make("Cancel"));
    
    mainmenu.pushMenuItem(StdMenuItemSubmenu::make("Custom or Multiplayer game", configmenu, MAIN_NEWGAMEMENU));
    
    if(FLAGS_startingPhase == -1)
      start = 0;
    else
      start = Coord(FLAGS_startingPhase);
    end_max = names.size() - 1;
    end = end_max;
    moneyexp = Coord(0.1133);
  }
  
  opts_res = getCurrentResolution();
  opts_aspect = getCurrentAspect();
  opts_fullscreen = getCurrentFullscreen();
  
  {
    {
      vector<pair<string, pair<int, int> > > resoptions;
      vector<pair<int, int> > resses = getResolutions();
      for(int i = 0; i < resses.size(); i++)
        resoptions.push_back(make_pair(StringPrintf("%dx%d", resses[i].first, resses[i].second), resses[i]));
      optionsmenu.pushMenuItem(StdMenuItemChooser<pair<int, int> >::make("Resolution", resoptions, &opts_res, bind(&InterfaceMain::opts_res_changed, this, _1)));
    }
    
    {
      vector<pair<string, bool> > onoff;
      onoff.push_back(make_pair("On", true));
      onoff.push_back(make_pair("Off", false));
      optionsmenu.pushMenuItem(StdMenuItemChooser<bool>::make("Fullscreen", onoff, &opts_fullscreen));
    }
    
    opts_aspect_chooser = StdMenuItemChooser<float>::make("Aspect ratio", gen_aspects(opts_res.first, opts_res.second), &opts_aspect);
    optionsmenu.pushMenuItem(opts_aspect_chooser);
    
    optionsmenu.pushMenuItem(StdMenuItemBack::make("Accept", OPTS_SETRES));
    optionsmenu.pushMenuItemAdjacent(StdMenuItemBack::make("Cancel"));
    
    mainmenu.pushMenuItem(StdMenuItemSubmenu::make("Options", &optionsmenu));
  }
  
  // we have to be really emphatic here
  // Real reason: The Resolution chooser will choose a resolution it likes. If we currently have a non-standard windowed resolution, it's not going to be happy and it'll reset to 640x480.
  // However, we need this to be accurate so that dragging to resize the screen works. So we set it here.
  // We set it above *also* so that the resolution chooser can, in fact, pick our real resolution if that is a standard resolution.
  // There is probably a better way to do this.
  opts_res = getCurrentResolution();
  opts_aspect = getCurrentAspect();
  opts_fullscreen = getCurrentFullscreen();
  
  mainmenu.pushMenuItem(StdMenuItemTrigger::make("Input test", MAIN_INPUTTEST));
  mainmenu.pushMenuItem(StdMenuItemTrigger::make("Exit", MAIN_EXIT));
  
  grid = false;
  inptest = false;
  
  escmenu.pushMenuItem(StdMenuItemBack::make("Return to game"));
  escmenu.pushMenuItem(StdMenuItemSubmenu::make("Options", &optionsmenu));
  escmenu.pushMenuItem(StdMenuItemTrigger::make("Main menu", ESCMENU_MAINMENU));
  escmenu.pushMenuItem(StdMenuItemTrigger::make("Quit", ESCMENU_QUIT));
  
  escmenuig.pushMenuItem(StdMenuItemBack::make("Return to game"));
  escmenuig.pushMenuItem(StdMenuItemTrigger::make("End game", ESCMENU_ENDGAME));
  escmenuig.pushMenuItem(StdMenuItemSubmenu::make("Options", &optionsmenu));
  escmenuig.pushMenuItem(StdMenuItemTrigger::make("Main menu", ESCMENU_MAINMENU));
  escmenuig.pushMenuItem(StdMenuItemTrigger::make("Quit", ESCMENU_QUIT));
  
  tick_sync_frame = 0;
  initIntroScreen(0, true);
}


// Cycle:
// <---     frames_for_full_cycle     --->
// < (100%)      ><preroll><fade_duration>


const int frames_for_full_cycle = FPS * 180;
const int fade_duration = FPS * 6;
const int preroll = FPS / 2;

void InterfaceMain::initIntroScreen(int id, bool first) {
  CHECK(id >= 0 && id < ARRAY_SIZE(introscreen));
  
  introscreen[id].reset(new GamePackage);
  introscreen_ais[id].clear();
  
  {
    introscreen[id]->players.resize(16);
    vector<const IDBFaction *> idbfa = ptrize(factionList());
    for(int i = 0; i < introscreen[id]->players.size(); i++) {
      int fid = unsync().choose(idbfa.size());
      introscreen[id]->players[i] = Player(idbfa[fid], 0, Money(0));
      idbfa.erase(idbfa.begin() + fid);
    }
  }
  
  introscreen[id]->game.initTitlescreen(&introscreen[id]->players, &unsync());
  
  for(int i = 0; i < introscreen[id]->players.size(); i++)
    introscreen_ais[id].push_back(smart_ptr<GameAiIntro>(new GameAiIntro()));
  
  if(!id && first)
    for(int i = 0; i < preroll; i++)
      introscreen[id]->runTickWithAi(vdc<GameAi*>(ptrize(introscreen_ais[id])), &unsync());
}

void InterfaceMain::tickIntroScreen() {
  vector<bool> tq = introScreenToTick();
  
  CHECK(tq.size() == ARRAY_SIZE(introscreen));
  
  for(int i = 0; i < tq.size(); i++) {
    if(tq[i]) {
      introscreen[i]->runTickWithAi(vdc<GameAi*>(ptrize(introscreen_ais[i])), &unsync());
      introscreen[i]->runTickWithAi(vdc<GameAi*>(ptrize(introscreen_ais[i])), &unsync());
    }
  }
  
  tick_sync_frame++;
  
  if(tick_sync_frame % frames_for_full_cycle == frames_for_full_cycle - preroll - fade_duration)
    initIntroScreen((tick_sync_frame / frames_for_full_cycle + 1) % 2);
}

void InterfaceMain::renderIntroScreen() const {
  vector<float> brite = introScreenBrightness();
  
  CHECK(brite.size() == ARRAY_SIZE(introscreen));
  
  bool run = false;
  
  for(int i = 0; i < brite.size(); i++) {
    if(brite[i] != 0.0) {
      CHECK(introscreen[i]);
      GfxWindow gfxw(getZoom(), brite[i]);
      if(run)
        clearStencil();
      run = true;
      introscreen[i]->renderToScreen();
    }
  }
}

vector<bool> InterfaceMain::introScreenToTick() const {
  vector<bool> tq;
  
  int finq = tick_sync_frame % frames_for_full_cycle;
  int ite = (tick_sync_frame / frames_for_full_cycle) % 2;
  
  for(int i = 0; i < 2; i++) {
    if(finq >= frames_for_full_cycle - preroll - fade_duration) {
      tq.push_back(true);
    } else if(i == ite) {
      tq.push_back(true);
    } else {
      tq.push_back(false);
    }
  }
  
  return tq;
}

vector<float> InterfaceMain::introScreenBrightness() const {
  vector<float> brite;
  int finq = tick_sync_frame % frames_for_full_cycle;
  int ite = (tick_sync_frame / frames_for_full_cycle) % 2;
  
  for(int i = 0; i < 2; i++) {
    if(finq >= (frames_for_full_cycle - fade_duration)) {
      float base_val;
      if(ite == i)
        base_val = (frames_for_full_cycle - finq) / (float)fade_duration;
      else
        base_val = (finq + fade_duration - frames_for_full_cycle) / (float)fade_duration;
      
      brite.push_back(sqrt(base_val));
    } else if(ite == i) {
      brite.push_back(1.0);
    } else {
      brite.push_back(0.0);
    }
  }
  
  if(!(accumulate(brite.begin(), brite.end(), 0.) >= 1)) {  // note: make sure this catches NaN also
    for(int i = 0; i < brite.size(); i++)
      dprintf("%d: %f\n", i, brite[i]);
    dprintf("%d, %d\n", finq, ite);
    dprintf("%d\n", finq >= (frames_for_full_cycle - fade_duration));
    dprintf("%f, %f\n", (frames_for_full_cycle - finq) / (float)fade_duration, (finq + fade_duration - frames_for_full_cycle) / (float)fade_duration);
    CHECK(0);
  }
  
  return brite;
}

InterfaceMain::InterfaceMain(const InputSnag &isnag) {
  tick_sync_frame = 0;
  inescmenu = false;
  mouseconf_cooldown = 0;
  init(isnag);
}

InterfaceMain::~InterfaceMain() {
  dprintf("Deleting metagame\n");
  game.reset(); // right now, we do this explicitly to help with debugging
  dprintf("Metagame deleted\n");
}

