#ifndef DNET_INPUT
#define DNET_INPUT

#include <vector>

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

inline bool operator<(const Button &lhs, const Button &rhs) {
  if(lhs.down != rhs.down) return lhs.down < rhs.down;
  if(lhs.up != rhs.up) return lhs.up < rhs.up;
  if(lhs.push != rhs.push) return lhs.push < rhs.push;
  if(lhs.release != rhs.release) return lhs.release < rhs.release;
  if(lhs.repeat != rhs.repeat) return lhs.repeat < rhs.repeat;
  if(lhs.dur != rhs.dur) return lhs.dur < rhs.dur;
  if(lhs.sincerep != rhs.sincerep) return lhs.sincerep < rhs.sincerep;
  return false;
}

inline bool operator==(const Button &lhs, const Button &rhs) {
  return !(lhs < rhs) && !(rhs < lhs);
}
inline bool operator!=(const Button &lhs, const Button &rhs) {
  return !(lhs == rhs);
}

class Controller {
public:
  float x;
  float y;
  Button u;
  Button d;
  Button l;
  Button r;
  vector<float> axes;
  vector<Button> keys;

  void newState(const Controller &nst);

  Controller();
};

// this is not meant to be meaningful
inline bool operator<(const Controller &lhs, const Controller &rhs) {
  if(lhs.x != rhs.x) return lhs.x < rhs.x;
  if(lhs.y != rhs.y) return lhs.y < rhs.y;
  if(lhs.u != rhs.u) return lhs.u < rhs.u;
  if(lhs.d != rhs.d) return lhs.d < rhs.d;
  if(lhs.l != rhs.l) return lhs.l < rhs.l;
  if(lhs.r != rhs.r) return lhs.r < rhs.r;
  if(lhs.keys != rhs.keys) return lhs.keys < rhs.keys;
  return false;
}

enum { KSAX_UDLR, KSAX_ABSOLUTE, KSAX_TANK, KSAX_END };
const char *const ksax_names[] = { "UDLR", "ABSOLUTE", "TANK" };
const int ksax_minaxis[] = {2, 2, 4};

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

#endif
