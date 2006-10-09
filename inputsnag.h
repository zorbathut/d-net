#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

vector<Controller> controls_init();
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
vector<Ai *> controls_ai();
bool controls_users();
bool controls_recordable();
void controls_shutdown();

int controls_primary_id();
string controls_availdescr(int cid);

#endif
