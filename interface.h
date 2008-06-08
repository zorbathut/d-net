#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "interface_stdmenu.h"
#include "metagame.h"

using namespace std;

using boost::function;

class Ai;
class GameAiIntro;

class InterfaceMain : boost::noncopyable {
  
  enum { STATE_DISCLAIMER, STATE_MAINMENU, STATE_PLAYING };
  enum { MAIN_NEWGAME, MAIN_INSTANTACTION, MAIN_INPUTTEST, MAIN_GRID, MAIN_EXIT, MAIN_NEWGAMEMENU, MAIN_OPTIONSMENU, OPTS_SETRES, ESCMENU_ENDGAME, ESCMENU_MAINMENU, ESCMENU_QUIT };
  enum { ESC_MAINMENU, ESC_EXIT };
  int interface_mode;
  
  bool inescmenu;
  int mouseconf_cooldown;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  scoped_ptr<Metagame> game;
  
  StdMenu mainmenu;
  
  StdMenu escmenu;
  StdMenu escmenuig;
  
  StdMenu optionsmenu;
  
  Coord start;
  Coord end;
  Coord end_max;
  bool onstart;
  
  Coord moneyexp;
  Coord start_clamp(const Coord &opt) const;
  Coord end_clamp(const Coord &opt) const;
  
  int rounds;
  
  // 0 represents "battle choice", 1-4 are the valid normal options
  int faction;
  bool faction_toggle;
  
  int aicount;
  
  pair<int, int> opts_res;
  bool opts_fullscreen;
  float opts_aspect;
  smart_ptr<StdMenuItemChooser<float> > opts_aspect_chooser;
  void opts_res_changed(pair<int, int> newres);
  
  vector<Keystates> kst;
  
  scoped_ptr<GamePackage> introscreen[2];
  vector<smart_ptr<GameAiIntro> > introscreen_ais[2];
  int tick_sync_frame;
  void initIntroScreen(int id, bool first = false);
  void tickIntroScreen();
  void renderIntroScreen() const;
  
  vector<bool> introScreenToTick() const;
  vector<float> introScreenBrightness() const;
  
  void init(const InputSnag &isnag);
  
public:

  bool tick(const InputState &control, RngSeed gameseed, InputSnag &is);
  void ai(const vector<Ai *> &ais, const vector<bool> &isHuman) const;
  bool isWaitingOnAi() const;
  void render(const InputSnag &is) const;

  void forceResize(int w, int h);

  void checksum(Adler32 *adl) const;

  InterfaceMain(const InputSnag &isnag);
  ~InterfaceMain();

};

#endif
