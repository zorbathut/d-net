#ifndef DNET_INPUT
#define DNET_INPUT

#include <vector>

#include "float.h"

using namespace std;

/*************
 * User input
 */
 
class Button {
public:
  bool down;
  bool up;
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
const int ksax_minaxis[] = {2, 2, 2}; // yeah okay shut up the code still works

vector<vector<vector<string> > > ksax_axis_names_gen();
const vector<vector<vector<string> > > ksax_axis_names = ksax_axis_names_gen(); // sigh.
const char *const ksax_descriptions[KSAX_END][2] = { {"Turn axis and", "movement axis"}, {"Tank goes in", "axis direction"}, {"Independent", "tread control"} };

enum { BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_LAST };
const char * const button_names[] = { "Fire/", "Weapon/" };

#define FIRECOUNT 2

class Keystates {
public:
  float ax[2];
  int axmode;
  Button u,d,l,r;
  Button fire;
  Button change;
  float udlrax[2];

  void nullMove();

  Keystates();
};

/*************
 * Utility funcs
 */

float deadzone(float t, float o, float absdead, float tdead);
Float2 deadzone(const Float2 &mov, float absdead, float tdead);

#endif
