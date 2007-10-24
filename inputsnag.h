#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "rng.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

pair<RngSeed, vector<Controller> > controls_init(RngSeed default_seed);
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
vector<Ai *> controls_ai();
bool controls_users();
bool controls_recordable();
void controls_shutdown();
class ControlShutdown {
public:
  ~ControlShutdown() {
    controls_shutdown();
  }
};

void controls_snag_next_checksum_set();

int controls_primary_id();

ControlConsts controls_getcc(int cid);

#endif
