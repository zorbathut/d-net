#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include <vector>

using namespace std;

#include "input.h"

class Ai;
class SDL_KeyboardEvent;

vector<Controller> controls_init();
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
vector<Ai *> controls_ai();
bool controls_users();
bool controls_recordable();
void controls_shutdown();

#endif
