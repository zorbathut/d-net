#ifndef DNET_INPUT
#define DNET_INPUT

#include "coord.h"

using namespace std;

/*************
 * User input
 */
 
struct Button {
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
void operator|=(Button &lhs, const Button &rhs);

void adler(Adler32 *adl, const Button &kst);

struct Controller {
  Coord2 menu;
  Button u;
  Button d;
  Button l;
  Button r;
  vector<Coord> axes;
  vector<Coord> lastaxes;
  vector<Button> keys;
  
  bool human;

  void newState(const Controller &nst);

  Controller();
};

bool operator==(const Controller &lhs, const Controller &rhs);

void adler(Adler32 *adl, const Controller &kst);

struct InputState {
  bool valid;
  
  vector<Controller> controllers;
  Button escape;
  
  bool confused;
  bool confused_mouse;
  
  InputState() { valid = true; confused = false; confused_mouse = false; }
};

enum AxisType { KSAX_STEERING, KSAX_ABSOLUTE, KSAX_TANK, KSAX_LAST };

enum { BUTTON_FIRE1, BUTTON_FIRE2, BUTTON_FIRE3, BUTTON_FIRE4, BUTTON_PRECISION, BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_LAST };
const int button_order[] = { BUTTON_ACCEPT, BUTTON_CANCEL, -1, BUTTON_PRECISION, BUTTON_FIRE1, BUTTON_FIRE2, BUTTON_FIRE3, BUTTON_FIRE4 };

struct Keystates {
  Coord ax[2];
  int axmode;
  Button fire[SIMUL_WEAPONS];
  
  Coord2 udlrax;
  Button u,d,l,r;
  Button precision;
  Button accept;
  Button cancel;
  
  Button accept_or_fire;

  void nullMove();

  Keystates();
};

struct CannedKeys {
  bool canned;
  bool is_second;
  
  int button_layout[ARRAY_SIZE(button_order)];
  
  vector<string> descriptive_text;
  
  CannedKeys() { canned = false; }
};

struct ControlConsts {
  vector<string> buttonnames;
  
  string description;
  string active_button;
  
  AxisType mode;
  
  CannedKeys ck;
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
