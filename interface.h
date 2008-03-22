#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

using namespace std;
using boost::function;

class Ai;
class GameAiIntro;

enum StdMenuCommand { SMR_NOTHING = -1, SMR_ENTER = -2, SMR_RETURN = -3 };

class StdMenuItem : boost::noncopyable {
public:

  virtual pair<StdMenuCommand, int> tickEntire(const Keystates &keys);
  virtual void renderEntire(const Float4 &bounds, bool obscure) const;
  
  virtual pair<StdMenuCommand, int> tickItem(const Keystates *keys) = 0;
  virtual float renderItemHeight() const;
  virtual float renderItemWidth(float tmx) const = 0;
  virtual void renderItem(const Float4 &bounds) const = 0; // ey is ignored

  virtual void checksum(Adler32 *adl) const = 0;

  StdMenuItem();
  virtual ~StdMenuItem();
};

class StdMenu {
  
  vector<vector<smart_ptr<StdMenuItem> > > items;
  int vpos;
  int hpos;
  
  bool inside;
  
public:

  void pushMenuItem(const smart_ptr<StdMenuItem> &site);
  void pushMenuItemAdjacent(const smart_ptr<StdMenuItem> &site);

  pair<StdMenuCommand, int> tick(const Keystates &keys);
  void render(const Float4 &bounds, bool obscure) const;

  void reset();

  void checksum(Adler32 *adl) const;

  StdMenu();

};

template<typename T> class StdMenuItemChooser;

class InterfaceMain : boost::noncopyable {
  
  enum { STATE_MAINMENU, STATE_PLAYING };
  enum { MAIN_NEWGAME, MAIN_INPUTTEST, MAIN_GRID, MAIN_EXIT, MAIN_NEWGAMEMENU, MAIN_OPTIONSMENU, OPTS_SETRES, ESCMENU_MAINMENU, ESCMENU_QUIT };
  enum { ESC_MAINMENU, ESC_EXIT };
  int interface_mode;
  
  bool inescmenu;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame *game;
  
  StdMenu mainmenu;
  StdMenu escmenu;
  StdMenu optionsmenu;
  
  Coord start;
  Coord end;
  bool onstart;
  
  Coord moneyexp;
  Coord start_clamp(const Coord &opt) const;
  Coord end_clamp(const Coord &opt) const;
  
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
  
  GamePackage *introscreen;
  vector<GameAiIntro *> introscreen_ais;
  
  void init();
  
public:

  bool tick(const InputState &control, RngSeed gameseed);
  void ai(const vector<Ai *> &ais, const vector<bool> &isHuman) const;
  bool isWaitingOnAi(const vector<bool> &humans) const;
  void render() const;

  void forceResize(int w, int h);

  void checksum(Adler32 *adl) const;

  InterfaceMain();
  ~InterfaceMain();

};

#endif
