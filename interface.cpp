
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
#include "vecedit.h"
#include "audio.h"

#include <boost/assign.hpp>

using namespace std;

DEFINE_bool(vecedit, false, "Vector editor (WARNING: nearly unusable)");
DEFINE_int(rounds_per_shop, 6, "How many rounds between each buying-things opportunity");
DEFINE_bool(auto_newgame, false, "Automatically enter New Game");
DEFINE_float(startingPhase, -1, "Starting phase override");
DEFINE_bool(showtanks, false, "Show-tank mode");

StdMenuItem StdMenuItem::makeStandardMenu(const string &text, int trigger) {
  StdMenuItem stim;
  stim.type = TYPE_TRIGGER;
  stim.name = text;
  stim.trigger = trigger;
  return stim;
}

StdMenuItem StdMenuItem::makeScale(const string &text, const vector<string> &labels, float *position) {
  StdMenuItem stim;
  stim.type = TYPE_SCALE;
  stim.name = text;
  stim.scale_labels = labels;
  stim.scale_position = position;
  return stim;
}

StdMenuItem StdMenuItem::makeRounds(const string &text, float *start, float *end, float *exp) {
  StdMenuItem stim;
  stim.type = TYPE_ROUNDS;
  stim.name = text;
  stim.rounds_start = start;
  stim.rounds_end = end;
  stim.rounds_exp = exp;
  return stim;
}

int StdMenuItem::tick(const Keystates &keys) {
  if(type == TYPE_TRIGGER) {
    if(keys.accept.push) {
      queueSound(S::accept);
      return trigger;
    }
  } else if(type == TYPE_SCALE) {
    if(keys.l.down)
      *scale_position -= 0.05;
    if(keys.r.down)
      *scale_position += 0.05;
  } else if(type == TYPE_ROUNDS) {
    if(keys.l.down)
      *rounds_exp *= 1.01;
    if(keys.r.down)
      *rounds_exp /= 1.01;
    *rounds_exp = clamp(*rounds_exp, 0.001, 2);
  } else {
    CHECK(0);
  }
  return -1; 
}

float StdMenuItem::render(float y) const {
  if(type == TYPE_TRIGGER) {
    drawText(name.c_str(), 4, Float2(2, y));
    return 6;
  } else if(type == TYPE_SCALE) {
    drawText(name.c_str(), 4, Float2(2, y));
    
    float xstart = 40;
    Float4 boundy = Float4(xstart, y, getZoom().ex - 4, y + 4);
    GfxWindow gfxw(boundy, 1.0);
    
    setZoomAround(Float4(*scale_position - 2, 0, *scale_position + 2, 0));
    
    float height = getZoom().y_span();
    
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
      path.push_back(Float2(*scale_position, height / 8));
      path.push_back(Float2(*scale_position + height / 16, height / 4));
      path.push_back(Float2(*scale_position, height / 2 - height / 8));
      path.push_back(Float2(*scale_position - height / 16, height / 4));
      drawLineLoop(path, height / 20);
    }
    return 6;
  } else if(type == TYPE_ROUNDS) {
    drawText(name.c_str(), 4, Float2(2, y));
    int rounds = int(ceil((*rounds_end - *rounds_start) * log(30) / *rounds_exp / 6)) * 6;
    float percentage = (exp(*rounds_exp) - 1) * 100;
    drawText(StringPrintf("%d (+%.2f%% cash/round)", rounds, percentage), 4, Float2(60, y));
    return 6;
  } else {
    CHECK(0);
  }
  CHECK(0);
}

StdMenuItem::StdMenuItem() { }

void StdMenu::pushMenuItem(const StdMenuItem &site) {
  items.push_back(site);
}

int StdMenu::tick(const Keystates &keys) {
  if(keys.u.repeat) {
    cpos--;
    queueSound(S::select);
  }
  if(keys.d.repeat) {
    cpos++;
    queueSound(S::select);
  }
  cpos = modurot(cpos, items.size());
  
  return items[cpos].tick(keys);
}

void StdMenu::render() const {
  setZoom(Float4(0, 0, 133.3333, 100));
  
  float y = 2;
  for(int i = 0; i < items.size(); i++) {
    if(i == cpos) {
      setColor(C::active_text);
    } else {
      setColor(C::inactive_text);
    }
    y += items[i].render(y);
  }
}

int StdMenu::currentItem() const {
  return cpos;
}

StdMenu::StdMenu() {
  cpos = 0;
}

#define GETDIFFERENCE_DEBUG 0 // Code used for checking the validity of getDifference :)

#if GETDIFFERENCE_DEBUG

#include "dvec2.h"
#include "parse.h" // fix these
#include <fstream>

vector<string> parseHackyFile(string fname) {
  ifstream ifs(fname.c_str());
  string tmp;
  vector<string> rv;
  while(ifs >> tmp)
    rv.push_back(tmp);
  return rv;
}

extern bool dumpBooleanDetail;

Controller ct;

bool InterfaceMain::tick(const vector< Controller > &control) {
  ct = control[controls_primary_id()];
  return false;
  
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
}

vector<Coord2> parseKvdCoords(const string &dat) {
  vector<string> tok = tokenize(dat, " \n\r");
  CHECK(tok.size() % 2 == 0);
  vector<Coord2> rv;
  for(int i = 0; i < tok.size(); i += 2)
    rv.push_back(Coord2(coordExplicit(tok[i]), coordExplicit(tok[i + 1])));
  return rv;
}

void InterfaceMain::render() const {
  
  dumpBooleanDetail = false;
  
  vector<Coord2> diff[2];
  
#if 0
  {
    ifstream dv("dvlr.txt");
    kvData kvd;
    CHECK(getkvData(dv, &kvd));
    CHECK(kvd.category == "coordfailure");
    
    diff[0] = parseKvdCoords(kvd.consume("lhs"));
    diff[1] = parseKvdCoords(kvd.consume("rhs"));
  }
#elif 1
  //dprintf("Frame!\n");
    
   string lhs[48] = {
     "ffffffa800000000", "ffffffd800000000",
     "ffffff8800000000", "ffffffd800000000",
     "ffffff8800000000", "0000002800000000",
     "ffffffa800000000", "0000002800000000",
     "ffffffb15be68000", "0000003835d24000",
     "ffffffa15be68000", "00000053ec4d8000",
     "ffffffe6a41a0000", "0000007bec4d0000",
     "fffffff6a419f000", "0000006035d20000",
     "000000095be6c000", "0000006035d20000",
     "000000195be6e000", "0000007bec4d0000",
     "0000005ea41a0000", "00000053ec4c8000",
     "0000004ea41a0000", "0000003835d1c000",
     "0000005800000000", "00000027ffff8000",
     "0000007800000000", "00000027ffff4000",
     "0000007800000000", "ffffffd7ffff4000",
     "0000005800000000", "ffffffd7ffff8000",
     "0000004ea4190000", "ffffffc7ca2d8000",
     "0000005ea4190000", "ffffffac13b28000",
     "000000195be58000", "ffffff8413b30000",
     "000000095be5c000", "ffffff9fca2e0000",
     "fffffff6a41a4000", "ffffff9fca2e0000",
     "ffffffe6a41a8000", "ffffff8413b30000",
     "ffffffa15be70000", "ffffffac13b28000",
     "ffffffb15be70000", "ffffffc7ca2d8000",
   };
   string rhs[8] = {
     "0000006400000000", "0000001400000000",
     "0000006400000000", "0000002800000000",
     "0000007800000000", "0000002800000000",
     "0000007800000000", "0000001400000000",
   };

  for(int i = 0; i < ARRAY_SIZE(lhs); i += 2)
    diff[0].push_back(Coord2(coordExplicit(lhs[i]), coordExplicit(lhs[i + 1])));
  for(int i = 0; i < ARRAY_SIZE(rhs); i += 2)
    diff[1].push_back(Coord2(coordExplicit(rhs[i]), coordExplicit(rhs[i + 1])));
#else
  vector<string> lhs = parseHackyFile("lhs.txt");
  vector<string> rhs = parseHackyFile("rhs.txt");
  
  for(int i = 0; i < lhs.size(); i += 2)
    diff[0].push_back(Coord2(coordExplicit(lhs[i]), coordExplicit(lhs[i + 1])));
  for(int i = 0; i < rhs.size(); i += 2)
    diff[1].push_back(Coord2(coordExplicit(rhs[i]), coordExplicit(rhs[i + 1])));
#endif

  vector<vector<Coord2> > res = getDifference(diff[0], diff[1]);
  
  //dprintf("%d\n", res.size());
  
  if(res.size()) {
    Coord4 bbox = getBoundBox(diff[1]);
    static pair<Coord2, Coord> zoom = make_pair(bbox.midpoint(), bbox.y_span() / 2 * Coord(1.1));
    
    if(ct.l.down)
      zoom.first.x -= zoom.second / 50;
    if(ct.r.down)
      zoom.first.x += zoom.second / 50;
    if(ct.u.down)
      zoom.first.y -= zoom.second / 50;
    if(ct.d.down)
      zoom.first.y += zoom.second / 50;
    
    if(ct.keys[0].down)
      zoom.second /= Coord(1.1);
    if(ct.keys[1].down)
      zoom.second *= Coord(1.1);
    
    //dprintf("%s", bbox.rawstr().c_str());
    setZoomCenter(0, 0, 1);

    for(int i = 0; i < res.size(); i++) {
      
      vector<Coord2> nres;
      for(int j = 0; j < res[i].size(); j++)
        nres.push_back((res[i][j] - zoom.first) / zoom.second);
  
      if(i == 0)
        setColor(1.0, 0.3, 0.3);
      else
        setColor(0.3, 1.0, 0.3);
      drawLineLoop(nres, 0.003);
      for(int j = 0; j < res[i].size(); j++) {
        drawCircle(nres[j].toFloat(), 0.03, 0.003);
        drawText(StringPrintf("%f", res[i][j].x.toFloat()), 0.05, Float2(nres[j].toFloat().x + 0.05, nres[j].toFloat().y + 0.05));
        drawText(StringPrintf("%f", res[i][j].y.toFloat()), 0.05, Float2(nres[j].toFloat().x + 0.05, nres[j].toFloat().y + 0.12));
        //drawText(res[j].x.rawstr(), 1, Float2(nres[i][j].toFloat().x + 1, nres[i][j].toFloat().y + 1));
        //drawText(res[j].y.rawstr(), 1, Float2(nres[i][j].toFloat().x + 1, nres[i][j].toFloat().y + 2.1));
      }
    }
    /*
    setColor(1.0, 1.0, 1.0);
    drawCircle(Float2(6.026120, -32.951839) * fin.second + Float2(fin.first.first, fin.first.second), 2.0, 0.1);
    drawCircle(Float2(6.904762, -33.830498) * fin.second + Float2(fin.first.first, fin.first.second), 2.0, 0.1);
    drawCircle(Float2(6.904793, -33.830513) * fin.second + Float2(fin.first.first, fin.first.second), 2.0, 0.1);*/
  }


  /*
  drawCircle(diff[0][3].toFloat() * fin.second + Float2(fin.first.first, fin.first.second), 2.0, 0.1);
  
  dprintf("%d\n", whichSide(Coord4(diff[1][1], diff[1][0]), diff[0][3]));
  dprintf("%d\n", whichSide(Coord4(diff[1][1], diff[1][0]), diff[0][2]));*/

  //CHECK(0);
  
};

#else

bool InterfaceMain::tick(const vector< Controller > &control, RngSeed gameseed) {
  if(FLAGS_showtanks)
    return false;
  
  StackString stp("Interface ticking");
  
  inptest_controls = control;
  
  if(kst.size() == 0) {
    CHECK(control.size() != 0);
    kst.resize(control.size());
  }
  
  CHECK(kst.size() == control.size());
  
  for(int i = 0; i < control.size(); i++) {
    CHECK(control[i].keys.size() >= 1);
    kst[i].u.newState(control[i].menu.y > .5);
    kst[i].d.newState(control[i].menu.y < -.5);
    kst[i].r.newState(control[i].menu.x > .5);
    kst[i].l.newState(control[i].menu.x < -.5);
    bool aButtonPushed = false;
    for(int j = 0; j < control[i].keys.size(); j++)
      if(control[i].keys[j].push)
        aButtonPushed = true;
    kst[i].accept.newState(aButtonPushed);
    kst[i].cancel.newState(false);
    for(int j = 0; j < SIMUL_WEAPONS; j++) {
      kst[i].fire[j].newState(false);
      kst[i].change[j].newState(false);
    }
  }

  if(interface_mode == STATE_MAINMENU) {
    int mrv = mainmenu.tick(kst[controls_primary_id()]);
    if(mrv == MAIN_NEWGAME || FLAGS_auto_newgame) {
      interface_mode = STATE_CONFIGURE;
    } else if(mrv == MAIN_EXIT) {
      return true;
    } else if(mrv == MAIN_INPUTTEST) {
      inptest = !inptest;
    } else if(mrv == MAIN_GRID) {
      grid = !grid;
    } else {
      CHECK(mrv == -1);
    }
  } else if(interface_mode == STATE_CONFIGURE) {
    int mrv = configmenu.tick(kst[controls_primary_id()]);
    start = clamp(start, 0, 6);
    end = clamp(end, 0, 6);
    if(start > end) {
      if(configmenu.currentItem() == 0) {
        start = end;
      } else if(configmenu.currentItem() == 1) {
        end = start;
      } else {
        CHECK(0); // what
      }
    }
    if(mrv == 1 || FLAGS_auto_newgame) {
      game = new Metagame(control.size(), Money((long long)(1000 * pow(30, start))), exp(moneyexp), FLAGS_rounds_per_shop, gameseed);
      interface_mode = STATE_PLAYING;
    }
  } else if(interface_mode == STATE_PLAYING) {
    if(game->runTick(control))
      interface_mode = STATE_MAINMENU;
  } else {
    CHECK(0);
  }
  
  return false;
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
  StackString stp("Interface AI");
  if(interface_mode == STATE_MAINMENU) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updatePregame();
  } else if(interface_mode == STATE_CONFIGURE) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updateSetup(configmenu.currentItem());
  } else if(interface_mode == STATE_PLAYING) {
    game->ai(ai);
  } else {
    CHECK(0);
  }
}

extern int lastFailedFrame;

void InterfaceMain::render() const {  
  if(FLAGS_showtanks) {
    setZoomVertical(0, 0, 200);
    setColor(1.0, 1.0, 1.0);
    float x = 0;
    float y = 0;
    const float xsize = 30;
    const float ysize = 30;
    for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++) {
      vector<Float2> vertices;
      for(int i = 0; i < itr->second.vertices.size(); i++)
        vertices.push_back(Float2(itr->second.vertices[i].y.toFloat(), -itr->second.vertices[i].x.toFloat()) + Float2(xsize, ysize) / 2 + Float2(x, y));
      drawLineLoop(vertices, 0.5);
      drawJustifiedText(strrchr(itr->first.c_str(), '.') + 1, 3, Float2(xsize / 2, ysize - 1) + Float2(x, y), TEXT_CENTER, TEXT_MAX);
      x += xsize;
      if(x + xsize > getZoom().ex) {
        y += ysize;
        x = 0;
      }
    }
    return;
  }
  
  StackString stp("Interface rendering");
  
  if(lastFailedFrame + 600 > frameNumber) {
    setZoomCenter(0, 0, 100);
    setColor(1.0, 1.0, 1.0);
    drawJustifiedText("A Lion", 30, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
  }
  
  if(interface_mode == STATE_MAINMENU) {
    mainmenu.render();
    setColor(C::inactive_text);
    drawText("Player one uses arrow keys and UIOJKL", 3, Float2(2, 30));
    drawText("Player two uses wasd and RTYFGH", 3, Float2(2, 34));
    drawText("Arrow keys to choose menu items, U to select", 3, Float2(2, 38));
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
      const float ysiz = crosshair + bord * 2;
      const float crosshairc = crosshair / 2;
      const float textsize = crosshair / 4;
      const float textrsize = textsize * 0.8;
      const int wid = int(800 / xsiz);
      const int textymax = int(crosshair / textsize);
      const int textxmax = 2;
      const float textxofs = crosshair / textxmax;
      
      const float boxthick = 0.5;
      for(int i = 0; i < inptest_controls.size(); i++) {
        const Controller &ct = inptest_controls[i];
        float x = (i % wid) * xsiz;
        float y = (i / wid) * ysiz + 300;
        Float4 chbox(x + bord, y + bord, x + bord + crosshair, y + bord + crosshair);
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
        drawCrosshair(Float2(ct.menu.x * crosshairc + bord + crosshairc + x, -ct.menu.y * crosshairc + bord + crosshairc + y), crosshair / 4, boxthick);
        setColor(C::active_text);
        float textx = x + bord * 3 + crosshair;
        float texty = y + bord;
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
    }
    if(isUnoptimized()) {
      setZoom(Float4(0, 0, 133.3333, 100));
      setColor(Color(1.0, 0.3, 0.3));
      drawText("Optimizations disabled!", 6, Float2(1, 1));
    }
  } else if(interface_mode == STATE_CONFIGURE) {
    configmenu.render();
    setColor(C::inactive_text);
    drawText(StringPrintf("%s starting cash", Money((long long)(1000 * pow(30, start))).textual().c_str()), 2, Float2(1, 97));
  } else if(interface_mode == STATE_PLAYING) {
    game->renderToScreen();
  } else {
    CHECK(0);
  }
};
#endif

InterfaceMain::InterfaceMain() {
  interface_mode = STATE_MAINMENU;
  mainmenu.pushMenuItem(StdMenuItem::makeStandardMenu("New game", MAIN_NEWGAME));
  mainmenu.pushMenuItem(StdMenuItem::makeStandardMenu("Input test", MAIN_INPUTTEST));
  mainmenu.pushMenuItem(StdMenuItem::makeStandardMenu("Grid toggle (useful for monitor sync)", MAIN_GRID));
  mainmenu.pushMenuItem(StdMenuItem::makeStandardMenu("Exit", MAIN_EXIT));
  
  vector<string> names = boost::assign::list_of("Junkyard")("Civilian")("Professional")("Military")("Exotic")("Experimental")("Ultimate");
  
  if(FLAGS_startingPhase == -1)
    start = 0;
  else
    start = FLAGS_startingPhase;
  end = names.size() - 1;
  moneyexp = 0.1133;
  
  configmenu.pushMenuItem(StdMenuItem::makeScale("Game start", names, &start));
  //configmenu.pushMenuItem(StdMenuItem::makeScale("Game end", names, &end));
  configmenu.pushMenuItem(StdMenuItem::makeRounds("Estimated rounds", &start, &end, &moneyexp));
  configmenu.pushMenuItem(StdMenuItem::makeStandardMenu("Begin", 1));
  
  grid = false;
  inptest = false;
  game = NULL;
}

InterfaceMain::~InterfaceMain() {
  dprintf("Deleting metagame\n");
  delete game;
  dprintf("Metagame deleted\n");
}

