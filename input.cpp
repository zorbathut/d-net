
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

float deadzone(float t, float o, float absdead, float tdead) {
  if(abs(t) < absdead)
    return 0;
  if(t*t + o*o < tdead*tdead)
    return 0;
  return t;
}

Float2 deadzone(const Float2 &mov, float absdead, float tdead) {
  return Float2(deadzone(mov.x, mov.y, absdead, tdead), deadzone(mov.y, mov.x, absdead, tdead));
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
      thisax.push_back("Move \"up\"");
      thisset.push_back(thisax);
    }
    {
      vector<string> thisax;
      thisax.push_back("Move \"right\"");
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
