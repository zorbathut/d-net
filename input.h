#ifndef DNET_INPUT
#define DNET_INPUT

#include "const.h"
#include "coord.h"

#include <string>

using namespace std;

/*************
 * User input
 */
 
class Button {
public:
  bool down;
  bool push;
  bool release;
  bool repeat;
  int dur;
  int sincerep;
  
  void newState(bool pushed);
  void newState(const Button &other);

  string stringize() const;

  Button();
};

bool operator==(const Button &lhs, const Button &rhs);

void adler(Adler32 *adl, const Button &kst);

class Controller {
public:
  Coord2 menu;
  Button u;
  Button d;
  Button l;
  Button r;
  vector<Coord> axes;
  vector<Coord> lastaxes;
  vector<Button> keys;

  void newState(const Controller &nst);

  Controller();
};

bool operator==(const Controller &lhs, const Controller &rhs);

enum { KSAX_STEERING, KSAX_ABSOLUTE, KSAX_TANK, KSAX_END };
const char *const ksax_names[] = { "Steering (default)", "Absolute", "Tank (advanced)" };

vector<vector<vector<string> > > ksax_axis_names_gen();
const vector<vector<vector<string> > > ksax_axis_names = ksax_axis_names_gen(); // sigh.
const char *const ksax_descriptions[KSAX_END][2] = { {"Turn axis and", "movement axis"}, {"Tank goes in", "stick direction"}, {"Independent", "tread control"} };
const int axis_groups[] = { 0, 0 };

enum { BUTTON_FIRE1, BUTTON_FIRE2, BUTTON_FIRE3, BUTTON_FIRE4, BUTTON_PRECISION, BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_LAST };
const char * const button_names[] = { "Fire weapon 1", "Fire weapon 2", "Fire weapon 3", "Fire weapon 4", "Slow movement", "Accept menu item key", "Change/abort menu key" };
const int button_groups[] = { 0, 0, 0, 0, 0, 1, 1 };

class Keystates {
public:
  Coord ax[2];
  int axmode;
  Button fire[SIMUL_WEAPONS];
  
  Coord2 udlrax;
  Button u,d,l,r;
  Button precision;
  Button accept;
  Button cancel;

  void nullMove();

  Keystates();
};

void adler(Adler32 *adl, const Keystates &kst);

/*************
 * Utility funcs
 */

enum { DEADZONE_ABSOLUTE, DEADZONE_CENTER };
Coord deadzone(Coord t, Coord o, int dztype, Coord tdead);
Coord2 deadzone(const Coord2 &mov, int dztype, Coord tdead);
Coord prepower(Coord in);

#endif
