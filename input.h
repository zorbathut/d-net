#ifndef DNET_INPUT
#define DNET_INPUT

#include "float.h"
#include "const.h"

#include <string>
#include <vector>

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

  Button();
};

class Controller {
public:
  Float2 menu;
  Button u;
  Button d;
  Button l;
  Button r;
  vector<float> axes;
  vector<Button> keys;

  void newState(const Controller &nst);

  Controller();
};

enum { KSAX_STEERING, KSAX_ABSOLUTE, KSAX_TANK, KSAX_END };
const char *const ksax_names[] = { "STEERING", "ABSOLUTE", "TANK" };

vector<vector<vector<string> > > ksax_axis_names_gen();
const vector<vector<vector<string> > > ksax_axis_names = ksax_axis_names_gen(); // sigh.
const char *const ksax_descriptions[KSAX_END][2] = { {"Turn axis and", "movement axis"}, {"Tank goes in", "stick direction"}, {"Independent", "tread control"} };
const int axis_groups[] = { 0, 0 };

enum { BUTTON_FIRE1, BUTTON_FIRE2, BUTTON_SWITCH1, BUTTON_SWITCH2, BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_LAST };
const char * const button_names[] = { "Fire weapon 1", "Fire weapon 2", "Switch weapon 1", "Switch weapon 2", "Accept menu item key", "Change/abort menu key" };
const int button_groups[] = { 0, 0, 0, 0, 1, 1 };

class Keystates {
public:
  float ax[2];
  int axmode;
  Button fire[SIMUL_WEAPONS];
  Button change[SIMUL_WEAPONS];
  
  Float2 udlrax;
  Button u,d,l,r;
  Button accept;
  Button cancel;

  void nullMove();

  Keystates();
};

/*************
 * Utility funcs
 */

enum { DEADZONE_ABSOLUTE, DEADZONE_CENTER };
float deadzone(float t, float o, int dztype, float tdead);
Float2 deadzone(const Float2 &mov, int dztype, float tdead);
float prepower(float in);

#endif
