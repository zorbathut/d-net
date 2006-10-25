#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"
#include "metagame.h"

using namespace std;

class Ai;

class StdMenu {
  
  vector<pair<string, int> > items;
  int cpos;
  
public:

  void pushMenuItem(const string &name, int triggeraction);

  int tick(const Keystates &keys);
  void render() const;

  StdMenu();

};

class InterfaceMain {
  
  enum { IFM_S_MAINMENU, IFM_S_PLAYING };
  enum { IFM_M_NEWGAME, IFM_M_INPUTTEST, IFM_M_GRID, IFM_M_EXIT };
  int interface_mode;
  
  bool grid;
  bool inptest;
  vector<Controller> inptest_controls;
  
  Metagame game;
  
  StdMenu mainmenu;
  
  vector< Keystates > kst;
  
public:

  bool tick(const vector< Controller > &control);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  InterfaceMain();
};

#endif
