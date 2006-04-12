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

enum { KSAX_UDLR, KSAX_ABSOLUTE, KSAX_TANK, KSAX_END };
const char *const ksax_names[] = { "UDLR", "ABSOLUTE", "TANK" };
const int ksax_minaxis[] = {2, 2, 2}; // yeah okay shut up the code still works

const char *const ksax_axis_names[KSAX_END][2] = { {"Turn", "Drive"}, {"X", "Y"}, {"Left", "Right"} };

class Keystates {
public:
  float ax[2];
  int axmode;
  Button u,d,l,r,f;
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
