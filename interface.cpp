
#include "interface.h"

#include "ai.h"
#include "args.h"
#include "gfx.h"
#include "metagame.h"
#include "player.h"
#include "vecedit.h"
#include "debug.h"

using namespace std;

DEFINE_bool(vecedit, false, "vector editor mode");
DEFINE_int(rounds_per_store, 6, "rounds between store enter");

class StdMenu {
  
  vector<pair<string, int> > items;
  int cpos;
  
public:

  void pushMenuItem(const string &name, int triggeraction);

  int tick(const Keystates &keys);
  void render() const;

  StdMenu();

};

void StdMenu::pushMenuItem(const string &name, int triggeraction) {
  items.push_back(make_pair(name, triggeraction));
}

int StdMenu::tick(const Keystates &keys) {
  if(keys.accept.down)
    return items[cpos].second;
  if(keys.u.repeat)
    cpos--;
  if(keys.d.repeat)
    cpos++;
  cpos += items.size();
  cpos %= items.size();
  return -1;
}

void StdMenu::render() const {
  setZoom(Float4(0, 0, 133.3333, 100));
  for(int i = 0; i < items.size(); i++) {
    if(i == cpos) {
      setColor(1.0, 1.0, 1.0);
    } else {
      setColor(0.5, 0.5, 0.5);
    }
    drawText(items[i].first.c_str(), 5, Float2(2, 2 + 6 * i));
  }
}

StdMenu::StdMenu() {
  cpos = 0;
}

class InterfaceMain {
  
  enum { IFM_S_MAINMENU, IFM_S_PLAYING };
  enum { IFM_M_NEWGAME, IFM_M_INPUTTEST, IFM_M_GRID, IFM_M_EXIT };
  int interface_mode;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame *game;
  
  StdMenu mainmenu;
  
  vector< Keystates > kst;
  
public:

  bool tick(const vector< Controller > &control);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  InterfaceMain();
  ~InterfaceMain();

};

#define GETDIFFERENCE_DEBUG 0 // Code used for checking the validity of getDifference :)

#if GETDIFFERENCE_DEBUG

include "dvec2.h"
include "parse.h" // fix these
include <fstream>


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
  ct = control[0];
  return false;
  
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
  
#if 1
  {
    ifstream dv("dvlr.txt");
    kvData kvd;
    CHECK(getkvData(dv, &kvd));
    CHECK(kvd.category == "coordfailure");
    
    diff[0] = parseKvdCoords(kvd.consume("lhs"));
    diff[1] = parseKvdCoords(kvd.consume("rhs"));
  }
#elif 0
  //dprintf("Frame!\n");
    
   string lhs[6] = {
     "00000002a98bb4a0", "00000001f6be3497",
     "fffffffdad7dd7a0", "000000025b2baed7",
     "ffffffffa8f673cc", "fffffffbae161c90",
   };
   string rhs[28] = {
     "fffffff827727300", "ffffff9c4eeabc00",
     "00000023054f2400", "ffffffa255310000",
     "0000004b7890d800", "ffffffbe64e39000",
     "00000061b91e0800", "ffffffeac8405d00",
     "000000600146d000", "0000001bfb9f8000",
     "00000046b5eeec00", "00000046b5eeec00",
     "0000001bfb9f8000", "000000600146d000",
     "ffffffeac8405d00", "00000061b91e0800",
     "ffffffbe64e39000", "0000004b7890d800",
     "ffffffa255310000", "00000023054f2400",
     "ffffff9d08ecb400", "fffffff1a696bd00",
     "ffffffb022ee1c00", "ffffffc3d188ec00",
     "ffffffd687da3600", "ffffffa500fdbc00",
     "0000000000000000", "0000000000000000",
   };

  for(int i = 0; i < sizeof(lhs) / sizeof(*lhs); i += 2)
    diff[0].push_back(Coord2(coordExplicit(lhs[i]), coordExplicit(lhs[i + 1])));
  for(int i = 0; i < sizeof(rhs) / sizeof(*rhs); i += 2)
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
  
  dprintf("%d\n", res.size());
  
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
    
    dprintf("%s", bbox.rawstr().c_str());
    setZoom(-1.25, -1.0, 1.0);

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
        drawCircle(nres[j], Coord(0.03), Coord(0.003));
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

bool InterfaceMain::tick(const vector< Controller > &control) {
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
  
  if(interface_mode == IFM_S_MAINMENU) {
    int mrv;
    mrv = mainmenu.tick(kst[0]);
    if(mrv == IFM_M_NEWGAME) {
      game = new Metagame(control.size(), FLAGS_rounds_per_store);
      interface_mode = IFM_S_PLAYING;
    } else if(mrv == IFM_M_EXIT) {
      return true;
    } else if(mrv == IFM_M_INPUTTEST) {
      inptest = !inptest;
    } else if(mrv == IFM_M_GRID) {
      grid = !grid;
    } else {
      CHECK(mrv == -1);
    }
  } else if(interface_mode == IFM_S_PLAYING) {
    if(game->runTick(control)) {
      interface_mode = IFM_S_MAINMENU;
    }
  } else {
    CHECK(0);
  }
  
  return false;
  
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
  StackString stp("Interface AI");
  if(interface_mode == IFM_S_MAINMENU) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updatePregame();
  } else if(interface_mode == IFM_S_PLAYING) {
    game->ai(ai);
  }
}

extern int lastFailedFrame;

void InterfaceMain::render() const {
  StackString stp("Interface rendering");
  
  if(lastFailedFrame + 600 > frameNumber) {
    setZoomCenter(0, 0, 100);
    setColor(1.0, 1.0, 1.0);
    drawJustifiedText("A Lion", 30, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
  }
  
  if(interface_mode == IFM_S_MAINMENU) {
    mainmenu.render();
    setColor(0.5, 0.5, 0.5);
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
      for(int i = 0; i < inptest_controls.size(); i++) {
        const Controller &ct = inptest_controls[i];
        float x = (i % wid) * xsiz;
        float y = (i / wid) * ysiz + 300;
        Float4 chbox(x + bord, y + bord, x + bord + crosshair, y + bord + crosshair);
        drawLine(Float4(chbox.sx, chbox.sy, chbox.sx + crosshair / 4, chbox.sy), 0.1);
        drawLine(Float4(chbox.sx, chbox.sy, chbox.sx, chbox.sy + crosshair / 4), 0.1);
        drawLine(Float4(chbox.sx, chbox.ey, chbox.sx + crosshair / 4, chbox.ey), 0.1);
        drawLine(Float4(chbox.sx, chbox.ey, chbox.sx, chbox.ey - crosshair / 4), 0.1);
        drawLine(Float4(chbox.ex, chbox.sy, chbox.ex - crosshair / 4, chbox.sy), 0.1);
        drawLine(Float4(chbox.ex, chbox.sy, chbox.ex, chbox.sy + crosshair / 4), 0.1);
        drawLine(Float4(chbox.ex, chbox.ey, chbox.ex - crosshair / 4, chbox.ey), 0.1);
        drawLine(Float4(chbox.ex, chbox.ey, chbox.ex, chbox.ey - crosshair / 4), 0.1);
        drawCrosshair(Float2(ct.menu.x * crosshairc + bord + crosshairc + x, -ct.menu.y * crosshairc + bord + crosshairc + y), crosshair / 4, 0.1);
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
  } else if(interface_mode == IFM_S_PLAYING) {
    game->renderToScreen();
  } else {
    CHECK(0);
  }
};
#endif

InterfaceMain::InterfaceMain() {
  interface_mode = IFM_S_MAINMENU;
  mainmenu.pushMenuItem("New game", IFM_M_NEWGAME);
  mainmenu.pushMenuItem("Input test", IFM_M_INPUTTEST);
  mainmenu.pushMenuItem("Grid toggle", IFM_M_GRID);
  mainmenu.pushMenuItem("Exit", IFM_M_EXIT);
  grid = false;
  inptest = false;
  game = NULL;
}

InterfaceMain::~InterfaceMain() {
  dprintf("Deleting metagame\n");
  delete game;
  dprintf("Metagame deleted\n");
}

InterfaceMain ifm;

void interfaceInit() {
  ifm = InterfaceMain();
}

bool interfaceRunTick(const vector< Controller > &control) {
  if(FLAGS_vecedit) {
    return vecEditTick(control[0]);
  } else {
    return ifm.tick(control);
  }
}

void interfaceRunAi(const vector<Ai *> &ais) {
  if(FLAGS_vecedit) {
    for(int i = 0; i < ais.size(); i++)
      CHECK(!ais[i]);
  } else {
    return ifm.ai(ais);
  }
}
  
void interfaceRenderToScreen() {
  if(FLAGS_vecedit) {
    vecEditRender();
  } else {
    ifm.render();
  }
}
