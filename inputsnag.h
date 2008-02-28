#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "rng.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

InputState controls_init();
void controls_key(const SDL_KeyboardEvent *key);
InputState controls_next();
vector<Ai *> controls_ai();
bool controls_users();
void controls_shutdown();
class ControlShutdown {
public:
  ControlShutdown() { } // this makes gcc shut up about unused variables
  ~ControlShutdown() {
    controls_shutdown();
  }
};

int controls_primary_id();
pair<int, int> controls_getType(int id);

ControlConsts controls_getcc(int cid);

#endif
