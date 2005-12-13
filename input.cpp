
#include "input.h"
#include "util.h"

/*************
 * User input
 */
 
Button::Button() {
  down = push = release = repeat = false;
  up = true;
  dur = 0;
  sincerep = 0;
}

void Button::newState(const Button &other) {
  newState(other.down);
}

void Button::newState(bool pushed) {
  if(pushed == down) {
    push = false;
    release = false;
  } else {
    dur = 0;
    sincerep = 0;
    if(pushed) {
      push = true;
    } else {
      release = true;
    }
    down = pushed;
    up = !pushed;
  }
  repeat = false;
  dur++;
  if(down) {
    if(sincerep % 10 == 0) {
      repeat = true;
    }
    sincerep++;
  }
}

void Controller::newState(const Controller &nst) {
  x = nst.x;
  y = nst.y;
  axes = nst.axes;
  u.newState(nst.u);
  d.newState(nst.d);
  l.newState(nst.l);
  r.newState(nst.r);
  CHECK(keys.size() == nst.keys.size());
  for(int i = 0; i < keys.size(); i++)
    keys[i].newState(nst.keys[i]);
}

Controller::Controller() {
  x = y = 0;
}

void Keystates::nullMove() {
  ax[0] = ax[1] = 0;
  u = d = l = r = Button();
}

Keystates::Keystates() {
  ax[0] = ax[1] = 0;
  axmode = KSAX_UDLR;
}



float deadzone(float t, float o, float absdead, float tdead) {
  if(abs(t) < absdead)
    return 0;
  if(t*t + o*o < tdead*tdead)
    return 0;
  return t;
}
