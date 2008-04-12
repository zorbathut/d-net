
#include "inputsnag.h"

#include "ai.h"
#include "args.h"
#include "dumper_registry.h"
#include "debug.h"
#include "util.h"
#include "smartptr.h"
#include "dumper.h"

#include <boost/assign/list_of.hpp>

#include <set>

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

using namespace std;

DEFINE_int(nullControllers, 0, "Null controllers to insert in front");

DEFINE_bool(treatAiAsHuman, false, "Treat AIs as humans for all game-modifying decisions");
REGISTER_bool(treatAiAsHuman);

enum { CIP_KEYBOARD, CIP_JOYSTICK, CIP_AI, CIP_NULL };

static vector<pair<int, int> > sources;
static vector<int> prerecorded;
static vector<SDL_Joystick *> joysticks;
static vector<smart_ptr<Ai> > ai;
static int primary_id;

static InputState last;
static InputState now;

const int playerone[] = { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RETURN };
const int playertwo[] = { SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_r, SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h, SDLK_x, SDLK_c, SDLK_v, SDLK_b, SDLK_n };

const int confusedkeys[] = { SDLK_RETURN, SDLK_SPACE, SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LMETA, SDLK_RMETA, SDLK_LSUPER, SDLK_RSUPER };

const int *const baseplayermap[2] = { playerone, playertwo };
const int baseplayersize[2] = { ARRAY_SIZE(playerone), ARRAY_SIZE(playertwo) };  

InputState controls_init(Dumper *dumper, bool allow_standard, int ais) {
  CHECK(sources.size() == 0);
  
  now.controllers.resize(FLAGS_nullControllers);
  prerecorded.resize(FLAGS_nullControllers);
  for(int i = 0; i < FLAGS_nullControllers; i++) {
    sources.push_back(make_pair((int)CIP_NULL, 0));
    now.controllers[i].axes.resize(2);
    now.controllers[i].keys.resize(4);
  }
  primary_id = FLAGS_nullControllers;
  
  if(dumper->is_replaying()) {
    CHECK(FLAGS_nullControllers == 0);
    
    InputState is = dumper->get_layout();
    
    now.controllers = is.controllers;
    prerecorded.resize(now.controllers.size(), true);
    sources = dumper->get_sources();
    primary_id = dumper->get_primary_id();
    
    CHECK(sources.size() == now.controllers.size());
    
  } else if(allow_standard) {
    
    // Keyboard init
    sources.push_back(make_pair((int)CIP_KEYBOARD, 0));
    sources.push_back(make_pair((int)CIP_KEYBOARD, 1));
    
    {
      Controller ct;
      ct.axes.resize(2);
      
      ct.keys.resize(baseplayersize[0] - 4);
      now.controllers.push_back(ct);
      
      ct.keys.resize(baseplayersize[1] - 4);
      now.controllers.push_back(ct);
    }
    
    // Joystick init
    dprintf("%d joysticks detected\n", SDL_NumJoysticks());
  
    for(int i = 0; i < SDL_NumJoysticks(); i++) {
      dprintf("Opening %d: %s\n", i, SDL_JoystickName(i));
      joysticks.push_back(SDL_JoystickOpen(i));
      
      CHECK(SDL_JoystickNumButtons(joysticks.back()) >= 1);
      dprintf("%d axes, %d buttons\n", SDL_JoystickNumAxes(joysticks.back()), SDL_JoystickNumButtons(joysticks.back()));
    }
    
    dprintf("Done opening joysticks\n");
    
    for(int i = 0; i < joysticks.size(); i++) {
      sources.push_back(make_pair((int)CIP_JOYSTICK, i));
      Controller ct;
      ct.axes.resize(SDL_JoystickNumAxes(joysticks[i]));
      ct.keys.resize(SDL_JoystickNumButtons(joysticks[i]));
      now.controllers.push_back(ct);
    }
    
    prerecorded.resize(sources.size(), false);
  } else {
    dprintf("Skipping humans\n");
  }
  
  CHECK(sources.size() != 0);
  CHECK(sources.size() == now.controllers.size());
  CHECK(prerecorded.size() == sources.size());
  
  {
    vector<bool> ihf = controls_human_flags();
    CHECK(ihf.size() == now.controllers.size());
    for(int i = 0; i < ihf.size(); i++)
      now.controllers[i].human = ihf[i];
  }
  
  last = now;
  
  dprintf("AIS is %d\n", ais);
  controls_set_ai_count(ais);
  
  dprintf("Primary ID is %d\n", primary_id);
  dumper->set_layout(now, sources, primary_id);
  
  return now;
}

// We're gonna have to do something funky here in dumper mode.
void controls_set_ai_count(int ct) {
  CHECK(ct >= 0 && ct <= 16);
  
  dprintf("changing ai count from %d to %d\n", controls_get_ai_count(), ct);

  // Technically this is O(n^2). I honestly don't care however.
  while(controls_get_ai_count() > ct) {
    CHECK(sources.back().first == CIP_AI);
    sources.pop_back();
    ai.pop_back();
    now.controllers.pop_back();
    last.controllers.pop_back();
    prerecorded.pop_back();
  }

  while(controls_get_ai_count() < ct) {
    sources.push_back(make_pair((int)CIP_AI, ai.size()));
    ai.push_back(smart_ptr<Ai>(new Ai()));
    ai.back()->updateIdle();
    Controller aic;
    aic.keys.resize(BUTTON_LAST);
    aic.axes.resize(2);
    aic.human = FLAGS_treatAiAsHuman;
    now.controllers.push_back(aic);
    last.controllers.push_back(aic);
    prerecorded.push_back(prerecorded[0]);  // yeah yeah
  }
  
  CHECK(now.controllers.size() == last.controllers.size());
}

int controls_get_ai_count(void) {
 
  int cic = 0;
  for(int i = 0; i < sources.size(); i++)
    if(sources[i].first == CIP_AI)
      cic++;
  
  return cic;
}

void controls_key(const SDL_KeyboardEvent *key) {
  if(key->type == SDL_KEYDOWN)
    for(int i = 0; i < ARRAY_SIZE(confusedkeys); i++)
      if(key->keysym.sym == confusedkeys[i])
        now.confused = true;
  
  bool *ps = NULL;
  for(int i = 0; i < sources.size(); i++) {
    if(sources[i].first != CIP_KEYBOARD)
      continue;
    CHECK(sources[i].second >= 0 && sources[i].second < ARRAY_SIZE(baseplayersize));
    for(int j = 0; j < baseplayersize[sources[i].second]; j++) {
      if(key->keysym.sym == baseplayermap[sources[i].second][j]) {
        if(j == 0)
          ps = &now.controllers[i].u.down;
        else if(j == 1)
          ps = &now.controllers[i].d.down;
        else if(j == 2)
          ps = &now.controllers[i].l.down;
        else if(j == 3)
          ps = &now.controllers[i].r.down;
        else
          ps = &now.controllers[i].keys[j - 4].down;
      }
    }
  }
  if(key->keysym.sym == SDLK_ESCAPE)
    ps = &now.escape.down;
  if(!ps)
    return;
  if(key->type == SDL_KEYUP)
    *ps = false;
  else if(key->type == SDL_KEYDOWN)
    *ps = true;
}

void controls_mouseclick() {
  now.confused_mouse = true;
}

InputState controls_next(Dumper *dumper) {
  StackString sst("Controls");
  
  dumper->get_layout(&now, &sources, &primary_id);
  
  if(dumper->is_replaying()) {
    now = dumper->read_input();
    if(!now.valid)
      return now;
    CHECK(now.controllers.size() == last.controllers.size());
  } else {
    SDL_JoystickUpdate();
    for(int i = 0; i < now.controllers.size(); i++) {
      if(sources[i].first == CIP_JOYSTICK) {
        int jstarget = sources[i].second;
        now.controllers[i].menu.x = SDL_JoystickGetAxis(joysticks[jstarget], 0) / 32768.0f;
        now.controllers[i].menu.y = -(SDL_JoystickGetAxis(joysticks[jstarget], 1) / 32768.0f);
        for(int j = 0; j < now.controllers[i].keys.size(); j++)
          now.controllers[i].keys[j].down = SDL_JoystickGetButton(joysticks[jstarget], j);
        for(int j = 0; j < now.controllers[i].axes.size(); j++) {
          int toggle = 1;
          if(j == 1 || j == 2)
            toggle *= -1;
          now.controllers[i].axes[j] = SDL_JoystickGetAxis(joysticks[jstarget], j) / 32768.0f * toggle;
        }
      } else if(sources[i].first == CIP_AI) {
        now.controllers[i] = ai[sources[i].second]->getNextKeys();
      } else if(sources[i].first == CIP_NULL) {
      } else if(sources[i].first == CIP_KEYBOARD) {
      }
    }
  }

  // now we update the parts that have to be implied
  
  for(int i = 0; i < now.controllers.size(); i++) {
    if(prerecorded[i])
      continue; // DENIED
    if(sources[i].first == CIP_KEYBOARD) {
      now.controllers[i].menu.x = now.controllers[i].r.down - now.controllers[i].l.down;
      now.controllers[i].menu.y = now.controllers[i].u.down - now.controllers[i].d.down;
      now.controllers[i].axes[0] = now.controllers[i].menu.x;
      now.controllers[i].axes[1] = now.controllers[i].menu.y;
    } else if(sources[i].first == CIP_JOYSTICK || sources[i].first == CIP_AI) {
      if(now.controllers[i].menu.x < -0.7) {
        now.controllers[i].r.down = false;
        now.controllers[i].l.down = true;
      } else if(now.controllers[i].menu.x > 0.7) {
        now.controllers[i].r.down = true;
        now.controllers[i].l.down = false;
      } else {
        now.controllers[i].r.down = false;
        now.controllers[i].l.down = false;
      }
      if(now.controllers[i].menu.y < -0.7) {
        now.controllers[i].u.down = false;
        now.controllers[i].d.down = true;
      } else if(now.controllers[i].menu.y > 0.7) {
        now.controllers[i].u.down = true;
        now.controllers[i].d.down = false;
      } else {
        now.controllers[i].u.down = false;
        now.controllers[i].d.down = false;
      }
      if(sources[i].first == CIP_AI) {
        now.controllers[i].axes.resize(2);
        now.controllers[i].axes[0] = now.controllers[i].menu.x;
        now.controllers[i].axes[1] = now.controllers[i].menu.y;
      }
    } else if(sources[i].first == CIP_NULL) {
      for(int j = 0; j < now.controllers[i].axes.size(); j++)
        now.controllers[i].axes[j] = 0;
    } else {
      CHECK(0);
    }
  }

  // now we do the deltas
  
  if(last.controllers.size() != now.controllers.size()) {
    dprintf("controller mismatch, %d vs %d\n", last.controllers.size(), now.controllers.size());
    CHECK(0);
  }
  
  for(int i = 0; i < now.controllers.size(); i++)
    last.controllers[i].newState(now.controllers[i]);
  last.escape.newState(now.escape);
  
  for(int i = 0; i < last.controllers.size(); i++) {
    CHECK(last.controllers[i].menu.x >= -1 && last.controllers[i].menu.x <= 1);
    CHECK(last.controllers[i].menu.y >= -1 && last.controllers[i].menu.y <= 1);
    
    // in an attempt to make dumpfiles smaller
    last.controllers[i].menu.x = round(last.controllers[i].menu.x * 128) / 128;
    last.controllers[i].menu.y = round(last.controllers[i].menu.y * 128) / 128;
    
    for(int j = 0; j < last.controllers[i].axes.size(); j++)
      CHECK(abs(last.controllers[i].axes[j]) <= 1);
  }
  
  last.confused = now.confused;
  last.confused_mouse = now.confused_mouse;
  
  now.confused = false;
  now.confused_mouse = false;
  
  return last;
}

vector<Ai *> controls_ai() {
  vector<Ai *> rv;
  for(int i = 0; i < sources.size(); i++) {
    if(sources[i].first == CIP_AI && !prerecorded[i])
      rv.push_back(ai[sources[i].second].get());
    else
      rv.push_back(NULL);
  }
  return rv;
}

vector<bool> controls_human_flags() {
  vector<bool> rv;
  for(int i = 0; i < sources.size(); i++) {
    if(sources[i].first == CIP_AI || sources[i].first == CIP_NULL)
      rv.push_back(FLAGS_treatAiAsHuman);
    else  // We let humans be considered human, even when replaying, since it changes some small behavior things.
      rv.push_back(true);
  }
  return rv;
}

bool controls_users() {
  CHECK(prerecorded.size());
  if(!(primary_id >= 0 && primary_id < prerecorded.size())) {
    dprintf("%d, %d\n", primary_id, prerecorded.size());
    CHECK(primary_id >= 0 && primary_id < prerecorded.size());
  }
  return !prerecorded[0] && (sources[primary_id].first == CIP_KEYBOARD || sources[primary_id].first == CIP_JOYSTICK);
}

void controls_shutdown() {
  CHECK(sources.size());
  for(int i = 0; i < joysticks.size(); i++)
    SDL_JoystickClose(joysticks[i]);
}

int controls_primary_id() {
  return primary_id;
}

pair<int, int> controls_getType(int id) {
  return sources[id];
}

ControlConsts controls_getcc(int cid) {
  ControlConsts rv;
  
  if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 0) {
    //rv.availdescr = "Available buttons are 7890UIOPJKL;M,./";
    rv.ck.canned = true;
    rv.ck.button_layout[0] = 6; // accept is 9
    rv.ck.button_layout[1] = 7; // cancel is 0
    rv.ck.button_layout[2] = 17; // axis for keys
    rv.ck.button_layout[3] = 15; // precision is /
    rv.ck.button_layout[4] = 0; // fire1 is u
    rv.ck.button_layout[5] = 1; // fire2 is i
    rv.ck.button_layout[6] = 2; // fire3 is o
    rv.ck.button_layout[7] = 3; // fire4 is p
    rv.ck.descriptive_text.push_back("Keyboard keys are currently fixed.");
    rv.ck.descriptive_text.push_back("");
    rv.ck.descriptive_text.push_back("Arrow keys to move");
    rv.ck.descriptive_text.push_back("O for menu accept, P for cancel");
    rv.ck.descriptive_text.push_back("7890 to fire your four weapons");
    rv.ck.descriptive_text.push_back("/ for precision mode");
    rv.ck.is_second = false;
    rv.active_button = "O";
  } else if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 1) {
    //rv.availdescr = "Available buttons are RTYFGHVBN";
    rv.ck.canned = true;
    rv.ck.button_layout[0] = 3; // accept is F
    rv.ck.button_layout[1] = 4; // cancel is G
    rv.ck.button_layout[2] = 11; // axis for keys
    rv.ck.button_layout[3] = 6; // precision is X
    rv.ck.button_layout[4] = 7; // fire1 is C
    rv.ck.button_layout[5] = 8; // fire2 is V
    rv.ck.button_layout[6] = 9; // fire3 is B
    rv.ck.button_layout[7] = 10; // fire4 is N
    rv.ck.descriptive_text.push_back("Keyboard keys are currently fixed.");
    rv.ck.descriptive_text.push_back("");
    rv.ck.descriptive_text.push_back("WASD to move");
    rv.ck.descriptive_text.push_back("F for menu accept, G for cancel");
    rv.ck.descriptive_text.push_back("CVBN to fire your four weapons");
    rv.ck.descriptive_text.push_back("X for precision mode");
    rv.ck.is_second = true;
    rv.active_button = "F";
  } else if(sources[cid].first == CIP_JOYSTICK) {
    //rv.availdescr = "Shoulder buttons recommended for weapons.";
    rv.active_button = "a button";
  } else {
    //rv.availdescr = "";
    rv.active_button = "a virtual button";
  }

  if(sources[cid].first == CIP_JOYSTICK) {
    rv.buttonnames = boost::assign::list_of("L2")("R2")("L1")("R1")("the left stick inwards")("A or X (for \"accept\")")("B or O (for \"cancel\")");
  } else {
    rv.buttonnames = boost::assign::list_of("Fire weapon 1")("Fire weapon 2")("Fire weapon 3")("Fire weapon 4")("Slow movement")("Accept menu item key")("Change/abort menu key");
  }
  CHECK(rv.buttonnames.size() == BUTTON_LAST);
  
  if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 0) {
    rv.description = "Right keyboard";
  } else if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 1) {
    rv.description = "Left keyboard";
  } else if(sources[cid].first == CIP_JOYSTICK) {
    rv.description = StringPrintf("Joystick #%d", sources[cid].second);
  } else if(sources[cid].first == CIP_NULL) {
    rv.description = StringPrintf("Null");
  } else if(sources[cid].first == CIP_AI) {
    rv.description = StringPrintf("AI #%d", sources[cid].second);
  } else {
    CHECK(0);
  }
  
  if(sources[cid].first == CIP_AI) {
    rv.mode = KSAX_ABSOLUTE;
  } else {
    rv.mode = KSAX_STEERING;
  }
  
  return rv;
}
