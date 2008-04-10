#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "rng.h"
#include "dumper.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

InputState controls_init(Dumper *dumper, bool allow_standard, int ais);
void controls_set_ai_count(int ct);
int controls_get_ai_count();
void controls_key(const SDL_KeyboardEvent *key);
void controls_mouseclick();
InputState controls_next(Dumper *dumper);
vector<Ai *> controls_ai();
vector<bool> controls_human_flags();
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
