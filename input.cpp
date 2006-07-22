
#include "input.h"

using namespace std;

/*************
 * User input
 */
 
Button::Button() {
  down = push = release = repeat = false;
  dur = 0;
  sincerep = 0;
}

void Button::newState(const Button &other) {
  newState(other.down);
}

void Button::newState(bool pushed) {
  push = false;
  release = false;
  if(pushed != down) {
    dur = 0;
    sincerep = 0;
    if(pushed) {
      push = true;
    } else {
      release = true;
    }
    down = pushed;
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
  menu = nst.menu;
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
  menu = Float2(0, 0);
}

void Keystates::nullMove() {
  ax[0] = ax[1] = 0;
  u = d = l = r = Button();
}

Keystates::Keystates() {
  ax[0] = ax[1] = 0;
  udlrax = Float2(0, 0);
  axmode = KSAX_STEERING;
}

float deadzone(float t, float o, int type, float amount) {
  if(type == DEADZONE_ABSOLUTE) {
    if(abs(t) < amount)
      return 0;
    float diff = (abs(t) - amount) / (1.0 - amount);
    if(t < 0)
      return -diff;
    return diff;
  } else if(type == DEADZONE_CENTER) {
    if(t*t + o*o < amount*amount)
      return 0;
    float diff = (sqrt(t*t + o*o) - amount) / (1.0 - amount);
    float ang = getAngle(Float2(t, o));
    return makeAngle(ang).x * diff;
  } else {
    CHECK(0);
  }
}

Float2 deadzone(const Float2 &mov, int type, float amount) {
  return Float2(deadzone(mov.x, mov.y, type, amount), deadzone(mov.y, mov.x, type, amount));
}

// This is all legacy stuff because we used to need two lines for some axis descriptions. I'm leaving it in because, hey, why not.
vector<vector<vector<string> > > ksax_axis_names_gen() {
  vector<vector<vector<string> > > rv;
  {
    vector<vector<string> > thisset;
    {
      vector<string> thisax;
      thisax.push_back("Turn right");
      thisset.push_back(thisax);
    }
    {
      vector<string> thisax;
      thisax.push_back("Drive forward");
      thisset.push_back(thisax);
    }
    rv.push_back(thisset);
  }
  {
    vector<vector<string> > thisset;
    {
      vector<string> thisax;
      thisax.push_back("Move \"right\"");
      thisset.push_back(thisax);
    }
    {
      vector<string> thisax;
      thisax.push_back("Move \"up\"");
      thisset.push_back(thisax);
    }
    rv.push_back(thisset);
  }
  {
    vector<vector<string> > thisset;
    {
      vector<string> thisax;
      thisax.push_back("Left tread forward");
      thisset.push_back(thisax);
    }
    {
      vector<string> thisax;
      thisax.push_back("Right tread forward");
      thisset.push_back(thisax);
    }
    rv.push_back(thisset);
  }
  return rv;
}
