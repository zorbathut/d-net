#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

using namespace std;

class Ai;

class StdMenuItem {
public:
  static StdMenuItem makeStandardMenu(const string &text, int trigger);

  int tick(const Keystates &keys);
  float render(float y) const;

private:
  enum { TYPE_TRIGGER, TYPE_LAST };
  
  int type;
  string name;

  int trigger;

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
  
  enum { IFM_S_MAINMENU, IFM_S_PLAYING };
  enum { IFM_M_NEWGAME, IFM_M_INPUTTEST, IFM_M_GRID, IFM_M_EXIT };
  int interface_mode;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame *game;
  
  StdMenu mainmenu;
  
  vector<Keystates> kst;
  
public:

  bool tick(const vector< Controller > &control, RngSeed gameseed);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  InterfaceMain();
  ~InterfaceMain();

};

#endif
