#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

using namespace std;

class Ai;
class GameAiIntro;

class StdMenuItem {
public:
  static StdMenuItem makeStandardMenu(const string &text, int trigger);
  static StdMenuItem makeScale(const string &text, const vector<string> &labels, Coord *position);
  static StdMenuItem makeRounds(const string &text, Coord *start, Coord *end, Coord *exp);
  static StdMenuItem makeOptions(const string &text, const vector<string> &labels, int *position);

  int tick(const Keystates &keys);
  float render(float y, bool mainmenu) const;

private:
  enum { TYPE_TRIGGER, TYPE_SCALE, TYPE_ROUNDS, TYPE_LAST };
  
  int type;
  string name;

  int trigger;
  
  vector<string> scale_labels;
  Coord *scale_posfloat;
  Coord scale_posint_approx;
  int *scale_posint;
  
  Coord *rounds_start;
  Coord *rounds_end;
  Coord *rounds_exp;

  StdMenuItem();
};

class StdMenu {
  
  vector<StdMenuItem> items;
  int cpos;
  
public:

  void pushMenuItem(const StdMenuItem &site);

  int tick(const Keystates &keys);
  void render(bool mainmenu) const;

  int currentItem() const;

  StdMenu();

};

class InterfaceMain : boost::noncopyable {
  
  enum { STATE_MAINMENU, STATE_CONFIGURE, STATE_PLAYING };
  enum { MAIN_NEWGAME, MAIN_INPUTTEST, MAIN_GRID, MAIN_EXIT };
  int interface_mode;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame *game;
  
  StdMenu mainmenu;
  StdMenu configmenu;
  
  Coord start;
  Coord end;
  Coord moneyexp;
  
  // 0 represents "battle choice", 1-4 are the valid normal options
  int faction;
  int faction_toggle;
  
  vector<Keystates> kst;
  
  GamePackage *introscreen;
  vector<GameAiIntro *> introscreen_ais;
  
  void init();
  
public:

  bool tick(const vector< Controller > &control, RngSeed gameseed);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  void checksum(Adler32 *adl) const;

  InterfaceMain();
  ~InterfaceMain();

};

#endif
