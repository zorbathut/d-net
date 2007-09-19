
#include "inputsnag.h"

#include "ai.h"
#include "args.h"
#include "debug.h"
#include "util.h"
#include "smartptr.h"
#include "adler32.h"

#ifdef OSX_FRAMEWORK_PREFIXES
  #include <SDL/SDL.h>
#else
  #include <SDL.h>
#endif

using namespace std;

DEFINE_string(readTarget, "", "File to replay from");

DEFINE_int(aiCount, 0, "Number of AIs");
DEFINE_int(nullControllers, 0, "Null controllers to insert in front");

enum { CIP_KEYBOARD, CIP_JOYSTICK, CIP_AI, CIP_PRERECORD, CIP_NULL };

static vector<pair<int, int> > sources;
static vector<SDL_Joystick *> joysticks;
static vector<smart_ptr<Ai> > ai;
static FILE *infile = NULL;

static vector<Controller> last;
static vector<Controller> now;

const int playerone[] = { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RETURN };
const int playertwo[] = { SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_r, SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h, SDLK_v, SDLK_b, SDLK_n };

const int *const baseplayermap[2] = { playerone, playertwo };
const int baseplayersize[2] = { ARRAY_SIZE(playerone), ARRAY_SIZE(playertwo) };  

pair<RngSeed, vector<Controller> > controls_init(RngSeed default_seed) {
  RngSeed rngs(default_seed);
  CHECK(sources.size() == 0);
  CHECK(FLAGS_readTarget == "" || FLAGS_aiCount == 0);
  if(FLAGS_readTarget != "") {
    dprintf("Reading state record from file %s\n", FLAGS_readTarget.c_str());
    infile = fopen(FLAGS_readTarget.c_str(), "rb");
    CHECK(infile);
    int dat;
    fread(&dat, 1, sizeof(dat), infile);
    CHECK(dat == 6);  // magic number
    fread(&rngs, 1, sizeof(rngs), infile);
    fread(&dat, 1, sizeof(dat), infile);
    dprintf("%d controllers\n", dat);
    now.resize(dat);
    for(int i = 0; i < now.size(); i++) {
      fread(&dat, 1, sizeof(dat), infile);
      dprintf("%d: %d buttons\n", i, dat);
      now[i].keys.resize(dat);
      fread(&dat, 1, sizeof(dat), infile);
      dprintf("%d: %d axes\n", i, dat);
      now[i].axes.resize(dat);
      sources.push_back(make_pair((int)CIP_PRERECORD, i));
    }
  } else if(FLAGS_aiCount) {
    now.resize(FLAGS_aiCount);
    dprintf("Creating AIs\n");
    for(int i = 0; i < FLAGS_aiCount; i++)
      ai.push_back(smart_ptr<Ai>(new Ai()));
    dprintf("AIs initialized\n");
    for(int i = 0; i < FLAGS_aiCount; i++) {
      sources.push_back(make_pair((int)CIP_AI, i));
      now[i].keys.resize(BUTTON_LAST);
      now[i].axes.resize(2);
    }
  } else {
    now.resize(FLAGS_nullControllers);
    for(int i = 0; i < FLAGS_nullControllers; i++) {
      sources.push_back(make_pair((int)CIP_NULL, 0));
      now[i].axes.resize(2);
      now[i].keys.resize(4);
    }
    
    // Keyboard init
    sources.push_back(make_pair((int)CIP_KEYBOARD, 0));
    sources.push_back(make_pair((int)CIP_KEYBOARD, 1));
    
    {
      Controller ct;
      ct.axes.resize(2);
      
      ct.keys.resize(baseplayersize[0] - 4);
      now.push_back(ct);
      
      ct.keys.resize(baseplayersize[1] - 4);
      now.push_back(ct);
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
      now.push_back(ct);
    }
  }
  CHECK(sources.size() != 0);
  CHECK(sources.size() == now.size());
  last = now;
  return make_pair(rngs, now);
}

void controls_key(const SDL_KeyboardEvent *key) {
  if(FLAGS_aiCount)
    return;
  bool *ps = NULL;
  for(int i = 0; i < ARRAY_SIZE(baseplayermap); i++) {
    for(int j = 0; j < baseplayersize[i]; j++) {
      if(key->keysym.sym == baseplayermap[i][j]) {
        if(j == 0)
          ps = &now[FLAGS_nullControllers + i].u.down;
        else if(j == 1)
          ps = &now[FLAGS_nullControllers + i].d.down;
        else if(j == 2)
          ps = &now[FLAGS_nullControllers + i].l.down;
        else if(j == 3)
          ps = &now[FLAGS_nullControllers + i].r.down;
        else
          ps = &now[FLAGS_nullControllers + i].keys[j-4].down;
      }
    }
  }
  if(!ps)
    return;
  if(key->type == SDL_KEYUP)
    *ps = false;
  else if(key->type == SDL_KEYDOWN)
    *ps = true;
}

vector<Controller> controls_next() {
  StackString sst("Controls");
  
  if(infile) {
    for(int i = 0; i < now.size(); i++) {
      fread(&now[i].menu.x, 1, sizeof(now[i].menu.x), infile);
      fread(&now[i].menu.y, 1, sizeof(now[i].menu.y), infile);
      fread(&now[i].u.down, 1, sizeof(now[i].u.down), infile);
      fread(&now[i].d.down, 1, sizeof(now[i].d.down), infile);
      fread(&now[i].l.down, 1, sizeof(now[i].l.down), infile);
      fread(&now[i].r.down, 1, sizeof(now[i].r.down), infile);
      for(int j = 0; j < now[i].keys.size(); j++)
        fread(&now[i].keys[j].down, 1, sizeof(now[i].keys[j].down), infile);
      for(int j = 0; j < now[i].axes.size(); j++)
        fread(&now[i].axes[j], 1, sizeof(now[i].axes[j]), infile);
    }
    if(feof(infile))
      return vector<Controller>();
    {
      int ct = 0;
      fread(&ct, 1, sizeof(ct), infile);
      CHECK(ct >= 1);
      reg_adler_ref_start();
      for(int i = 0; i < ct; i++) {
        unsigned long ite;
        fread(&ite, 1, sizeof(ite), infile);
        reg_adler_ref_item(ite);
      }
    }
    CHECK(!feof(infile));
  } else if(FLAGS_aiCount) {
    for(int i = 0; i < FLAGS_aiCount; i++)
      now[i] = ai[i]->getNextKeys();
  } else {
    SDL_JoystickUpdate();
    for(int i = 0; i < now.size(); i++) {
      if(sources[i].first == CIP_JOYSTICK) {
        int jstarget = sources[i].second;
        now[i].menu.x = SDL_JoystickGetAxis(joysticks[jstarget], 0) / 32768.0f;
        now[i].menu.y = -(SDL_JoystickGetAxis(joysticks[jstarget], 1) / 32768.0f);
        for(int j = 0; j < now[i].keys.size(); j++)
          now[i].keys[j].down = SDL_JoystickGetButton(joysticks[jstarget], j);
        for(int j = 0; j < now[i].axes.size(); j++) {
          int toggle = 1;
          if(j == 1 || j == 2)
            toggle *= -1;
          now[i].axes[j] = SDL_JoystickGetAxis(joysticks[jstarget], j) / 32768.0f * toggle;
        }
      }
    }
  }

  // Now we update the parts that have to be implied
  
  for(int i = 0; i < now.size(); i++) {
    if(sources[i].first == CIP_KEYBOARD) {
      now[i].menu.x = now[i].r.down - now[i].l.down;
      now[i].menu.y = now[i].u.down - now[i].d.down;
      now[i].axes[0] = now[i].menu.x;
      now[i].axes[1] = now[i].menu.y;
    } else if(sources[i].first == CIP_JOYSTICK || sources[i].first == CIP_AI) {
      if(now[i].menu.x < -0.7) {
        now[i].r.down = false;
        now[i].l.down = true;
      } else if(now[i].menu.x > 0.7) {
        now[i].r.down = true;
        now[i].l.down = false;
      } else {
        now[i].r.down = false;
        now[i].l.down = false;
      }
      if(now[i].menu.y < -0.7) {
        now[i].u.down = false;
        now[i].d.down = true;
      } else if(now[i].menu.y > 0.7) {
        now[i].u.down = true;
        now[i].d.down = false;
      } else {
        now[i].u.down = false;
        now[i].d.down = false;
      }
      if(sources[i].first == CIP_AI) {
        now[i].axes.resize(2);
        now[i].axes[0] = now[i].menu.x;
        now[i].axes[1] = now[i].menu.y;
      }
    } else if(sources[i].first == CIP_PRERECORD || sources[i].first == CIP_NULL) {
    } else {
      CHECK(0);
    }
  }

  // Now we do the deltas
  
  for(int i = 0; i < now.size(); i++)
    last[i].newState(now[i]);
  
  for(int i = 0; i < last.size(); i++) {
    CHECK(last[i].menu.x >= -1 && last[i].menu.x <= 1);
    CHECK(last[i].menu.y >= -1 && last[i].menu.y <= 1);
  }
  
  return last;
  
}

vector<Ai *> controls_ai() {
  if(FLAGS_aiCount) {
    vector<Ai *> ais;
    for(int i = 0; i < ai.size(); i++)
      ais.push_back(ai[i].get());
    return ais;
  } else {
    return vector<Ai *>(now.size());
  }
}

bool controls_users() {
  return !infile && !FLAGS_aiCount;
}

bool controls_recordable() {
  return FLAGS_readTarget == "";
}
void controls_shutdown() {
  for(int i = 0; i < joysticks.size(); i++)
    SDL_JoystickClose(joysticks[i]);
  if(infile)
    fclose(infile);
}

void controls_snag_next_checksum_set() {
  if(infile) {
    int ct = 0;
    fread(&ct, 1, sizeof(ct), infile);
    if(feof(infile)) {
      reg_adler_ref_nullity();
      return;
    }
    CHECK(ct >= 1);
    reg_adler_ref_start();
    for(int i = 0; i < ct; i++) {
      unsigned long ite;
      fread(&ite, 1, sizeof(ite), infile);
      reg_adler_ref_item(ite);
    }
  }
}

int controls_primary_id() {
  return FLAGS_nullControllers;
}

string controls_availdescr(int cid) {
  if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 0)
    return "Available buttons are 7890UIOPJKL;M,./";
  else if(sources[cid].first == CIP_KEYBOARD && sources[cid].second == 1)
    return "Available buttons are RTYFGHVBN";
  else if(sources[cid].first == CIP_JOYSTICK)
    return "Shoulder buttons recommended for weapons.";
  else
    return "";
}
