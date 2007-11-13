#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

#include <boost/function.hpp>

using namespace std;
using boost::function;

class Ai;
class GameAiIntro;

class StdMenuItem;

enum StdMenuCommand { SMR_NOTHING = -1, SMR_ENTER = -2, SMR_RETURN = -3 };

class StdMenu {
  
  vector<vector<StdMenuItem> > items;
  int vpos;
  int hpos;
  
  bool inside;
  
public:

  void pushMenuItem(const StdMenuItem &site);
  void pushMenuItemAdjacent(const StdMenuItem &site);

  pair<StdMenuCommand, int> tick(const Keystates &keys);
  void render(const Float4 &bounds, bool obscure) const;

  void reset();

  StdMenu();

};

class StdMenuItem {
public:
  static StdMenuItem makeTrigger(const string &text, int trigger);

  struct ScaleDisplayer {
    vector<string> labels;
    const Coord *start;
    const Coord *end;
    const bool *onstart;
    bool mini;
    
    void render(float pos) const;
    
    ScaleDisplayer(const vector<string> &labels, const Coord *start, const Coord *end, const bool *onstart, bool mini) : labels(labels), start(start), end(end), onstart(onstart), mini(mini) { };
    ScaleDisplayer(const vector<string> &labels) : labels(labels), start(NULL), end(NULL), onstart(NULL) { };
    ScaleDisplayer() { };
  };
  
  static StdMenuItem makeScale(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos);
  static StdMenuItem makeRounds(const string &text, Coord *start, Coord *end, Coord *exp);
  static StdMenuItem makeOptions(const string &text, int *position, const ScaleDisplayer &sds);
  static StdMenuItem makeSubmenu(const string &text, StdMenu menu, int signal = SMR_NOTHING);
  static StdMenuItem makeBack(const string &text);

  pair<StdMenuCommand, int> tickEntire(const Keystates &keys);
  void renderEntire(const Float4 &bounds, bool obscure) const;
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemHeight() const;
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const; // ey is ignored

private:
  enum { TYPE_TRIGGER, TYPE_SCALE, TYPE_ROUNDS, TYPE_SUBMENU, TYPE_BACK, TYPE_LAST };
  
  int type;
  string name;

  int trigger;
  
  Coord *scale_posfloat;
  int *scale_posint;
  Coord scale_posint_approx;
  function<Coord (const Coord &)> scale_munge;
  ScaleDisplayer scale_displayer;
  bool scale_selected_val;
  bool *scale_selected_pos;
  
  Coord *rounds_start;
  Coord *rounds_end;
  Coord *rounds_exp;
  
  StdMenu submenu;
  int submenu_signal;

  StdMenuItem();
};

class InterfaceMain : boost::noncopyable {
  
  enum { STATE_MAINMENU, STATE_PLAYING };
  enum { MAIN_NEWGAME, MAIN_INPUTTEST, MAIN_GRID, MAIN_EXIT, MAIN_NEWGAMEMENU };
  enum { ESC_MAINMENU, ESC_EXIT };
  int interface_mode;
  
  bool escmenu;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame *game;
  
  StdMenu mainmenu;
  
  StdMenu escmenuitem;
  
  Coord start;
  Coord end;
  bool onstart;
  
  Coord moneyexp;
  Coord start_clamp(const Coord &opt) const;
  Coord end_clamp(const Coord &opt) const;
  
  // 0 represents "battle choice", 1-4 are the valid normal options
  int faction;
  int faction_toggle;
  
  vector<Keystates> kst;
  
  GamePackage *introscreen;
  vector<GameAiIntro *> introscreen_ais;
  
  void init();
  
public:

  bool tick(const InputState &control, RngSeed gameseed);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  void checksum(Adler32 *adl) const;

  InterfaceMain();
  ~InterfaceMain();

};

#endif
