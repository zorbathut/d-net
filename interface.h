#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

using namespace std;

class Ai;

class StdMenuItem {
public:
  static StdMenuItem makeStandardMenu(const string &text, int trigger);
  static StdMenuItem makeScale(const string &text, const vector<string> &labels, float *position);

  int tick(const Keystates &keys);
  float render(float y) const;

private:
  enum { TYPE_TRIGGER, TYPE_SCALE, TYPE_LAST };
  
  int type;
  string name;

  int trigger;
  
  vector<string> scale_labels;
  float *scale_position;

  StdMenuItem();
};

class StdMenu {
  
  vector<StdMenuItem> items;
  int cpos;
  
public:

  void pushMenuItem(const StdMenuItem &site);

  int tick(const Keystates &keys);
  void render() const;

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
  
  float start;
  float end;
  float expamount;
  
  vector<Keystates> kst;
  
public:

  bool tick(const vector< Controller > &control, RngSeed gameseed);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  InterfaceMain();
  ~InterfaceMain();

};

#endif
