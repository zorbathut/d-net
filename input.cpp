
#include "input.h"

#include "adler32_util.h"

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

string Button::stringize() const {
  string rv;
  if(down) {
    rv = "D";
  } else {
    rv = "U";
  }
  
  if(push)
    rv += "P";
  
  if(release)
    rv += "R";
  
  if(repeat)
    rv += "E";
  
  return rv;
};

bool operator==(const Button &lhs, const Button &rhs) {
  return lhs.down == rhs.down && lhs.push == rhs.push && lhs.release == rhs.release && lhs.repeat == rhs.repeat && lhs.dur == rhs.dur && lhs.sincerep == rhs.sincerep;
}

void adler(Adler32 *adl, const Button &kst) {
  adler(adl, kst.down);
  adler(adl, kst.push);
  adler(adl, kst.release);
  adler(adl, kst.repeat);
  adler(adl, kst.dur);
  adler(adl, kst.sincerep);
}

void Controller::newState(const Controller &nst) {
  menu = nst.menu;
  lastaxes = axes;
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
  menu = Coord2(0, 0);
}

bool operator==(const Controller &lhs, const Controller &rhs) {
  return lhs.menu == rhs.menu && lhs.u == rhs.u && lhs.d == rhs.d && lhs.l == rhs.r && lhs.r == rhs.r && lhs.axes == rhs.axes && lhs.lastaxes == rhs.lastaxes && lhs.keys == rhs.keys;
}

void adler(Adler32 *adl, const Controller &kst) {
  adler(adl, kst.menu);
  adler(adl, kst.u);
  adler(adl, kst.d);
  adler(adl, kst.l);
  adler(adl, kst.r);
  adler(adl, kst.axes);
  adler(adl, kst.lastaxes);
  adler(adl, kst.keys);
}

void Keystates::nullMove() {
  ax[0] = ax[1] = 0;
  u = d = l = r = Button();
}

Keystates::Keystates() {
  ax[0] = ax[1] = 0;
  udlrax = Coord2(0, 0);
  axmode = KSAX_STEERING;
}

void adler(Adler32 *adl, const Keystates &kst) {
  adler(adl, kst.ax);
  adler(adl, kst.axmode);
  adler(adl, kst.fire);
  adler(adl, kst.udlrax);
  adler(adl, kst.u);
  adler(adl, kst.d);
  adler(adl, kst.l);
  adler(adl, kst.r);
  adler(adl, kst.precision);
  adler(adl, kst.accept);
  adler(adl, kst.cancel);
}

Coord deadzone(Coord t, Coord o, int type, Coord amount) {
  if(type == DEADZONE_ABSOLUTE) {
    if(abs(t) < amount)
      return 0;
    Coord diff = (abs(t) - amount) / (1 - amount);
    if(t < 0)
      return -diff;
    return diff;
  } else if(type == DEADZONE_CENTER) {
    if(t*t + o*o < amount*amount)
      return 0;
    Coord diff = (sqrt(t*t + o*o) - amount) / (1 - amount);
    Coord ang = getAngle(Coord2(t, o));
    return makeAngle(ang).x * diff;
  } else {
    CHECK(0);
  }
}

Coord2 deadzone(const Coord2 &mov, int type, Coord amount) {
  return Coord2(deadzone(mov.x, mov.y, type, amount), deadzone(mov.y, mov.x, type, amount));
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

Coord prepower(Coord x) {
  if(x < 0)
    return -prepower(abs(x));
  CHECK(x >= 0);
  return pow(x, 3.0f);
}
